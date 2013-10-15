/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
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

#pragma once

#include <QObject>
#include <QDir>

extern "C"
{
#include <libotr/proto.h>
#include <libotr/message.h>
}

#include <interfaces/iinfo.h>
#include <interfaces/iplugin2.h>
#include <interfaces/core/ihookproxy.h>
#include <interfaces/azoth/iproxyobject.h>
#include <interfaces/azoth/imessage.h>

class QTranslator;

namespace LeechCraft
{
namespace Azoth
{
class ICLEntry;

namespace OTRoid
{
	class Plugin : public QObject
				 , public IInfo
				 , public IPlugin2
	{
		Q_OBJECT
		Q_INTERFACES (IInfo IPlugin2)

		IProxyObject *AzothProxy_;

		OtrlUserState UserState_;
		OtrlMessageAppOps OtrOps_;

		QHash<QObject*, QAction*> Entry2Action_;
		QHash<QAction*, QObject*> Action2Entry_;

		QHash<QObject*, QString> Msg2OrigText_;

		QDir OtrDir_;

		bool IsGenerating_ = false;

#if OTRL_VERSION_MAJOR >= 4
		QTimer *PollTimer_;
#endif
	public:
		void Init (ICoreProxy_ptr);
		void SecondInit ();
		QByteArray GetUniqueID () const;
		void Release ();
		QString GetName () const;
		QString GetInfo () const;
		QIcon GetIcon () const;

		QSet<QByteArray> GetPluginClasses () const;

		int IsLoggedIn (const QString& accId, const QString& entryId);
		void InjectMsg (const QString& accId, const QString& entryId,
				const QString& msg, bool hidden, IMessage::Direction,
				IMessage::MessageType = IMessage::MTChatMessage);
		void InjectMsg (ICLEntry *entry,
				const QString& msg, bool hidden, IMessage::Direction,
				IMessage::MessageType = IMessage::MTChatMessage);
		void Notify (const QString& accId, const QString& entryId,
				Priority, const QString& title,
				const QString& primary, const QString& secondary);
		void WriteFingerprints ();
		QString GetAccountName (const QString& accId);
		QString GetVisibleEntryName (const QString& accId, const QString& entryId);

		void CreatePrivkey (const char*, const char*);

#if OTRL_VERSION_MAJOR >= 4
		void SetPollTimerInterval (unsigned int seconds);
#endif
	private:
		const char* GetOTRFilename (const QString&) const;
		void CreateActions (QObject*);
	public slots:
		void initPlugin (QObject*);

		void hookEntryActionAreasRequested (LeechCraft::IHookProxy_ptr proxy,
				QObject *action,
				QObject *entry);
		void hookEntryActionsRemoved (LeechCraft::IHookProxy_ptr proxy,
				QObject *entry);
		void hookEntryActionsRequested (LeechCraft::IHookProxy_ptr proxy,
				QObject *entry);
		void hookGotMessage (LeechCraft::IHookProxy_ptr proxy,
				QObject *message);
		void hookMessageCreated (LeechCraft::IHookProxy_ptr proxy,
				QObject *chatTab,
				QObject *message);
	private slots:
		void handleOtrAction ();

#if OTRL_VERSION_MAJOR >= 4
		void pollOTR ();
#endif
	signals:
		void gotEntity (const LeechCraft::Entity&);
	};
}
}
}
