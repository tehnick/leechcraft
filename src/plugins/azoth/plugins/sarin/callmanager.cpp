/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#include "callmanager.h"
#include <QFuture>
#include <util/sll/futures.h>
#include "toxthread.h"
#include "util.h"
#include "threadexceptions.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Sarin
{
	CallManager::CallManager (ToxThread *thread, Tox *tox, QObject *parent)
	: QObject { parent }
	, Thread_ { thread }
	, ToxAv_ { toxav_new (tox, 64), &toxav_kill }
	{
		toxav_register_callstate_callback (ToxAv_.get (),
				[] (void*, int32_t callIdx, void *udata)
				{
					static_cast<CallManager*> (udata)->HandleIncomingCall (callIdx);
				},
				av_OnInvite,
				this);
		toxav_register_audio_recv_callback (ToxAv_.get (),
				[] (ToxAv*, int32_t callIdx, int16_t *frames, int size, void *udata)
				{
					static_cast<CallManager*> (udata)->HandleAudio (callIdx, frames, size);
				},
				this);
	}

	QFuture<CallManager::InitiateResult> CallManager::InitiateCall (const QByteArray& pkey)
	{
		return Thread_->ScheduleFunction ([this, pkey] (Tox *tox) -> InitiateResult
				{
					const auto id = GetFriendId (tox, pkey);
					if (id < 0)
					{
						qWarning () << Q_FUNC_INFO
								<< "unable to get user ID for"
								<< pkey;
						throw UnknownFriendException { tr ("Unable to get user ID.") };
					}

					int32_t callIdx = 0;
					auto res = toxav_call (ToxAv_.get (), &callIdx, id, &av_DefaultSettings, 15);
					if (res < 0)
					{
						qWarning () << Q_FUNC_INFO
								<< "unable to initiate call:"
								<< res;
						throw CallInitiateException { res };
					}

					res = toxav_prepare_transmission (ToxAv_.get (), callIdx, av_jbufdc, av_VADd, false);
					if (res < 0)
					{
						qWarning () << Q_FUNC_INFO
								<< "unable to prepare transmission:"
								<< res;
						throw CallInitiateException { res };
					}

					return { callIdx, av_DefaultSettings };
				});
	}

	QFuture<CallManager::WriteResult> CallManager::WriteData (int32_t callIdx, const QByteArray& data)
	{
		return Thread_->ScheduleFunction ([this, data, callIdx] (Tox*) -> WriteResult
				{
					const auto perFrame = av_DefaultSettings.audio_frame_duration * av_DefaultSettings.audio_sample_rate * av_DefaultSettings.audio_channels / 1000;
					const auto dataShift = perFrame * sizeof (int16_t);
					qDebug () << Q_FUNC_INFO << data.size () << perFrame;

					int currentPos = 0;
					for (; currentPos + dataShift < static_cast<uint> (data.size ()); currentPos += dataShift)
					{
						uint8_t prepared [RTP_PAYLOAD_SIZE] = { 0 };
						const auto size = toxav_prepare_audio_frame (ToxAv_.get (), callIdx,
								prepared, RTP_PAYLOAD_SIZE,
								reinterpret_cast<const int16_t*> (data.constData () + currentPos), perFrame);
						if (size < 0)
						{
							qWarning () << Q_FUNC_INFO
									<< "unable to prepare frame";
							throw FramePrepareException { size };
						}

						const auto rc = toxav_send_audio (ToxAv_.get (), callIdx, prepared, size);
						if (rc)
						{
							qWarning () << Q_FUNC_INFO
									<< "unable to send frame of size"
									<< size
									<< "; rc:"
									<< rc;
							throw FrameSendException { rc };
						}
						qDebug () << "sent frame of size" << size;
					}

					return { currentPos, data.mid (currentPos) };
				});
	}

	QFuture<CallManager::AcceptCallResult> CallManager::AcceptCall (int32_t callIdx)
	{
		return Thread_->ScheduleFunction ([this, callIdx] (Tox*) -> AcceptCallResult
				{
					ToxAvCSettings settings;
					auto rc = toxav_get_peer_csettings (ToxAv_.get (), callIdx, 0, &settings);
					if (rc != ErrorNone)
					{
						qWarning () << Q_FUNC_INFO
								<< "unable to get peer settings for call"
								<< callIdx
								<< rc;
						throw CallAnswerException { rc };
					}

					if ((rc = toxav_answer (ToxAv_.get (), callIdx, &settings)))
					{
						qWarning () << Q_FUNC_INFO
								<< "unable to answer the call"
								<< callIdx
								<< rc;
						throw CallAnswerException { rc };
					}

					return { settings };
				});
	}

	void CallManager::HandleIncomingCall (int32_t callIdx)
	{
		const auto friendNum = toxav_get_peer_id (ToxAv_.get (), callIdx, 0);
		if (friendNum < 0)
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to get friend ID for call"
					<< callIdx;
			return;
		}

		Util::ExecuteFuture ([this, friendNum] { return Thread_->GetFriendPubkey (friendNum); },
				[this, callIdx] (const QByteArray& pubkey) { HandleIncomingCall (pubkey, callIdx); },
				this);
	}

	void CallManager::HandleIncomingCall (const QByteArray& pubkey, int32_t callIdx)
	{
		ToxAvCSettings settings;
		const auto rc = toxav_get_peer_csettings (ToxAv_.get (), callIdx, 0, &settings);
		if (rc != ErrorNone)
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to get peer settings";
			return;
		}

		if (settings.call_type == TypeVideo)
		{
			qWarning () << Q_FUNC_INFO
					<< "video calls are unsupported for now";
			return;
		}

		emit gotIncomingCall (pubkey, callIdx);
	}

	void CallManager::HandleAudio (int32_t call, int16_t *frames, int size)
	{
		qDebug () << Q_FUNC_INFO << call << size;
		const auto& data = QByteArray { reinterpret_cast<char*> (frames), static_cast<int> (size * sizeof (int16_t)) };
		emit gotFrame (call, data);
	}
}
}
}
