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

#include "accountthread.h"
#include <QMutexLocker>
#include <QtDebug>
#include "account.h"
#include "accountthreadworker.h"
#include "core.h"
#include "taskqueuemanager.h"

namespace LeechCraft
{
namespace Snails
{
	AccountThread::AccountThread (bool isListening, Account *parent)
	: A_ { parent }
	, IsListening_ { isListening }
	{
	}

	void AccountThread::AddTask (const TaskQueueItem& item)
	{
		qDebug () << Q_FUNC_INFO;
		QMutexLocker guard { &QueueMutex_ };

		if (QueueManager_)
			QueueManager_->AddTasks ({ item });
		else
			PendingQueue_ << item;
		qDebug () << "done";
	}

	void AccountThread::run ()
	{
		W_ = new AccountThreadWorker { IsListening_, A_ };
		ConnectSignals ();

		{
			QMutexLocker guard { &QueueMutex_ };
			QueueManager_ = new TaskQueueManager { W_ };
			QueueManager_->AddTasks (PendingQueue_);
			PendingQueue_.clear ();
		}

		QThread::run ();

		delete QueueManager_;
		delete W_;
	}

	void AccountThread::ConnectSignals ()
	{
		connect (W_,
				SIGNAL (gotProgressListener (ProgressListener_g_ptr)),
				A_,
				SIGNAL (gotProgressListener (ProgressListener_g_ptr)));

		connect (W_,
				SIGNAL (gotMsgHeaders (QList<Message_ptr>, QStringList)),
				A_,
				SLOT (handleMsgHeaders (QList<Message_ptr>, QStringList)));
		connect (W_,
				SIGNAL (messageBodyFetched (Message_ptr)),
				A_,
				SLOT (handleMessageBodyFetched (Message_ptr)));
		connect (W_,
				SIGNAL (gotUpdatedMessages (QList<Message_ptr>, QStringList)),
				A_,
				SLOT (handleGotUpdatedMessages (QList<Message_ptr>, QStringList)));
		connect (W_,
				SIGNAL (gotOtherMessages (QList<QByteArray>, QStringList)),
				A_,
				SLOT (handleGotOtherMessages (QList<QByteArray>, QStringList)));
		connect (W_,
				SIGNAL (gotMessagesRemoved (QList<QByteArray>, QStringList)),
				A_,
				SLOT (handleMessagesRemoved (QList<QByteArray>, QStringList)));

		connect (W_,
				SIGNAL (gotFolders (QList<QStringList>)),
				A_,
				SLOT (handleGotFolders (QList<QStringList>)));
		connect (W_,
				SIGNAL (folderSyncFinished (QStringList, QByteArray)),
				A_,
				SLOT (handleFolderSyncFinished (QStringList, QByteArray)));

		connect (W_,
				SIGNAL (gotEntity (LeechCraft::Entity)),
				&Core::Instance (),
				SIGNAL (gotEntity (LeechCraft::Entity)));
	}
}
}
