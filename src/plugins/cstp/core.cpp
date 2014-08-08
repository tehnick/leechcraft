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

#include "core.h"
#include <stdexcept>
#include <numeric>
#include <algorithm>
#include <boost/logic/tribool.hpp>
#include <QDir>
#include <QTimer>
#include <QMetaType>
#include <QTextCodec>
#include <QStringList>
#include <QtDebug>
#include <QRegExp>
#include <QDesktopServices>
#include <interfaces/entitytesthandleresult.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/ijobholder.h>
#include <interfaces/an/constants.h>
#include <interfaces/idownload.h>
#include <util/util.h>
#include <util/sll/prelude.h>
#include <util/xpc/notificationactionhandler.h>
#include <util/xpc/util.h>
#include "task.h"
#include "xmlsettingsmanager.h"
#include "addtask.h"

Q_DECLARE_METATYPE (QNetworkReply*)

extern "C"
{
	#ifdef Q_OS_WIN32
		#include <stdio.h>
		static const int LC_FILENAME_MAX = FILENAME_MAX;
	#else
		#include <limits.h>
		static const int LC_FILENAME_MAX = NAME_MAX;
	#endif
}

namespace LeechCraft
{
namespace CSTP
{
	Core::Core ()
	: Headers_ { "URL", tr ("State"), tr ("Progress") }
	, SaveScheduled_ (false)
	, Toolbar_ (0)
	{
		setObjectName ("CSTP Core");
		qRegisterMetaType<std::shared_ptr<QFile>> ("std::shared_ptr<QFile>");
		qRegisterMetaType<QNetworkReply*> ("QNetworkReply*");

		ReadSettings ();
	}

	Core& Core::Instance ()
	{
		static Core core;
		return core;
	}

	void Core::Release ()
	{
		writeSettings ();
	}

	void Core::SetCoreProxy (ICoreProxy_ptr proxy)
	{
		CoreProxy_ = proxy;

		NetworkAccessManager_ = proxy->GetNetworkAccessManager ();
		connect (NetworkAccessManager_,
				SIGNAL (finished (QNetworkReply*)),
				this,
				SLOT (finishedReply (QNetworkReply*)));
	}

	ICoreProxy_ptr Core::GetCoreProxy () const
	{
		return CoreProxy_;
	}

	void Core::SetToolbar (QToolBar *widget)
	{
		Toolbar_ = widget;
	}

	void Core::ItemSelected (const QModelIndex& i)
	{
		Selected_ = i;
	}

	namespace
	{
		QString MakeFilename (const QUrl& entity)
		{
			QFileInfo fileInfo (entity.toString (QUrl::RemoveFragment));
			QString file = fileInfo.fileName ();
			if (file.length () >= LC_FILENAME_MAX)
			{
				QString extension (fileInfo.completeSuffix ());
				QString baseName (fileInfo.baseName ());

				// at least one character for file name and one for dot
				if (extension.length () > LC_FILENAME_MAX - 2)
				// in most cases there will be trash, but its hard to assume
				// how long extension could be. For example odf.tar.bz2
					extension.resize (LC_FILENAME_MAX - 2);
				if ((baseName.length () + extension.length ()) > (LC_FILENAME_MAX - 1))
					baseName.resize (LC_FILENAME_MAX - 1 - extension.length ());
				file = baseName + '.' + extension;
			}

			if (file.isEmpty ())
				file = QString ("index_%1")
					.arg (QDateTime::currentDateTime ().toString (Qt::ISODate));
			static const QRegExp restrictedChars (",|=|;|:|\\[|\\]|\\\"|\\*|\\?|&|\\||\\\\|/|(?:^LPT\\d$)|(?:^COM\\d$)|(?:^PRN$)|(?:^AUX$)|(?:^CON$)|(?:^NUL$)");
			static const QString replaceWith ('_');
			file.replace (restrictedChars, replaceWith);
			if (file != fileInfo.fileName ())
					qWarning () << Q_FUNC_INFO
							<< fileInfo.fileName ()
							<< "was corrected to:"
							<< file;
			return file;
		}

		QString MakeFilename (const Entity& e)
		{
			const QFileInfo fi (e.Location_);
			if (!fi.isDir ())
				return fi.fileName ();

			if (e.Additional_.contains ("Filename"))
				return e.Additional_ ["Filename"].toString ();

			const auto& url = e.Entity_.toUrl ();
			return MakeFilename (url.isValid () ? url : e.Additional_ ["SourceURL"].toUrl ());
		}
	}

	int Core::AddTask (const Entity& e)
	{
		auto url = e.Entity_.toUrl ();
		const auto& urlList = e.Entity_.value<QList<QUrl>> ();
		QNetworkReply *rep = e.Entity_.value<QNetworkReply*> ();
		const auto& tags = e.Additional_ [" Tags"].toStringList ();

		const QFileInfo fi (e.Location_);
		const auto& dir = fi.isDir () ? e.Location_ : fi.dir ().path ();
		const auto& file = MakeFilename (e);

		if (rep)
			return AddTask (rep,
					dir,
					file,
					QString (),
					tags,
					e.Parameters_);

		AddTask::Task task
		{
			url,
			dir,
			file,
			{}
		};

		if (e.Parameters_ & LeechCraft::FromUserInitiated &&
				e.Location_.isEmpty ())
		{
			CSTP::AddTask at (url, e.Location_);
			if (at.exec () == QDialog::Rejected)
				return -1;

			task = at.GetTask ();
		}

		if (!urlList.isEmpty ())
		{
			for (const auto& url : urlList)
				AddTask (url,
						dir,
						MakeFilename (url),
						{},
						tags,
						e.Additional_,
						e.Parameters_);

			return -1;
		}

		if (!dir.isEmpty ())
			return AddTask (task.URL_,
					task.LocalPath_,
					task.Filename_,
					task.Comment_,
					tags,
					e.Additional_,
					e.Parameters_);

		return -1;
	}

	void Core::KillTask (int id)
	{
		for (int i = 0, size = ActiveTasks_.size (); i != size; ++i)
			if (static_cast<int> (ActiveTasks_ [i].ID_) == id)
			{
				removeTriggered (i);
				return;
			}
		qWarning () << Q_FUNC_INFO
			<< "not found"
			<< id
			<< ActiveTasks_.size ();
	}

	int Core::AddTask (QNetworkReply *rep,
			const QString& path,
			const QString& filename,
			const QString& comment,
			const QStringList& tags,
			LeechCraft::TaskParameters tp)
	{
		TaskDescr td;
		td.Task_.reset (new Task (rep));

		QDir dir (path);
		td.File_.reset (new QFile (QDir::cleanPath (dir.filePath (filename))));
		td.Comment_ = comment;
		td.Parameters_ = tp;
		td.Tags_ = tags;

		return AddTask (td);
	}

	int Core::AddTask (const QUrl& url,
			const QString& path,
			const QString& filename,
			const QString& comment,
			const QStringList& tags,
			const QVariantMap& params,
			LeechCraft::TaskParameters tp)
	{
		TaskDescr td;

		td.Task_.reset (new Task (url, params));

		QDir dir (path);
		td.File_.reset (new QFile (QDir::cleanPath (dir.filePath (filename))));
		td.Comment_ = comment;
		td.Parameters_ = tp;
		td.Tags_ = tags;

		return AddTask (td);
	}

	int Core::AddTask (TaskDescr& td)
	{
		td.ErrorFlag_ = false;
		td.ID_ = CoreProxy_->GetID ();

		if (td.File_->exists ())
		{
			boost::logic::tribool remove = false;
			emit fileExists (&remove);
			if (remove)
			{
				if (!td.File_->resize (0))
				{
					QString msg = tr ("Could not truncate file ") +
						td.File_->errorString ();
					qWarning () << Q_FUNC_INFO << msg;
					emit error (msg);
				}
			}
			else if (!remove);
			else
			{
				CoreProxy_->FreeID (td.ID_);
				return -1;
			}
		}

		if (td.Parameters_ & Internal)
			td.Task_->ForbidNameChanges ();

		connect (td.Task_.get (),
				SIGNAL (done (bool)),
				this,
				SLOT (done (bool)));
		connect (td.Task_.get (),
				SIGNAL (updateInterface ()),
				this,
				SLOT (updateInterface ()));

		beginInsertRows (QModelIndex (), rowCount (), rowCount ());
		ActiveTasks_.push_back (td);
		endInsertRows ();
		ScheduleSave ();
		if (!(td.Parameters_ & LeechCraft::NoAutostart))
			startTriggered (rowCount () - 1);
		return td.ID_;
	}

	qint64 Core::GetDone (int pos) const
	{
		return TaskAt (pos).Task_->GetDone ();
	}

	qint64 Core::GetTotal (int pos) const
	{
		return TaskAt (pos).Task_->GetTotal ();
	}

	bool Core::IsRunning (int pos) const
	{
		return TaskAt (pos).Task_->IsRunning ();
	}

	qint64 Core::GetTotalDownloadSpeed () const
	{
		return std::accumulate (ActiveTasks_.begin (), ActiveTasks_.end (), 0,
				[] (qint64 acc, const Core::TaskDescr& td)
					{ return acc + td.Task_->GetSpeed (); });
	}

	namespace
	{
		EntityTestHandleResult CheckUrl (const QUrl& url, const Entity& e)
		{
			if (!url.isValid ())
				return {};

			const QString& scheme = url.scheme ();
			if (scheme == "file")
				return (!(e.Parameters_ & FromUserInitiated) && !(e.Parameters_ & IsDownloaded)) ?
						EntityTestHandleResult (EntityTestHandleResult::PHigh) :
						EntityTestHandleResult ();
			else
			{
				const QStringList schemes { "http", "https" };
				return schemes.contains (url.scheme ()) ?
						EntityTestHandleResult (EntityTestHandleResult::PIdeal) :
						EntityTestHandleResult ();
			}
		}
	}

	EntityTestHandleResult Core::CouldDownload (const Entity& e)
	{
		if (e.Entity_.value<QNetworkReply*> ())
			return EntityTestHandleResult (EntityTestHandleResult::PHigh);

		const auto& url = e.Entity_.toUrl ();
		const auto& urlList = e.Entity_.value<QList<QUrl>> ();
		if (url.isValid ())
			return CheckUrl (url, e);
		else if (!urlList.isEmpty ())
		{
			const auto& results = Util::Map (urlList,
					[&e] (const QUrl& url) { return CheckUrl (url, e); });
			const auto minPos = std::min_element (results.begin (), results.end (),
					[] (const EntityTestHandleResult& left, const EntityTestHandleResult& right)
					{
						return left.HandlePriority_ < right.HandlePriority_;
					});
			return minPos == results.end () ?
					EntityTestHandleResult {} :
					*minPos;
		}
		else
			return {};
	}

	QAbstractItemModel* Core::GetRepresentationModel ()
	{
		return this;
	}

	QNetworkAccessManager* Core::GetNetworkAccessManager () const
	{
		return NetworkAccessManager_;
	}

	bool Core::HasFinishedReply (QNetworkReply *rep) const
	{
		return FinishedReplies_.contains (rep);
	}

	void Core::RemoveFinishedReply (QNetworkReply *rep)
	{
		FinishedReplies_.remove (rep);
	}

	int Core::columnCount (const QModelIndex&) const
	{
		return Headers_.size ();
	}

	QVariant Core::data (const QModelIndex& index, int role) const
	{
		if (!index.isValid ())
			return {};

		if (index.row () >= static_cast<int> (ActiveTasks_.size ()))
			return {};

		if (role == Qt::DisplayRole)
		{
			const auto& td = TaskAt (index.row ());
			const auto& task = td.Task_;
			switch (index.column ())
			{
			case HURL:
				return task->GetURL ();
			case HState:
			{
				if (td.ErrorFlag_)
					return task->GetErrorString ();

				if (!task->IsRunning ())
					return QVariant ();

				qint64 done = task->GetDone (),
						total = task->GetTotal ();
				double speed = task->GetSpeed ();

				qint64 rem = (total - done) / speed;

				return tr ("%1 (ETA: %2)")
					.arg (task->GetState ())
					.arg (Util::MakeTimeFromLong (rem));
			}
			case HProgress:
			{
				qint64 done = task->GetDone ();
				qint64 total = task->GetTotal ();
				int progress = total ? done * 100 / total : 0;
				if (done > -1)
				{
					if (total > -1)
						return QString (tr ("%1% (%2 of %3 at %4)"))
							.arg (progress)
							.arg (Util::MakePrettySize (done))
							.arg (Util::MakePrettySize (total))
							.arg (Util::MakePrettySize (task->GetSpeed ()) + tr ("/s"));
					else
						return QString ("%1")
							.arg (Util::MakePrettySize (done));
				}
				else
					return QString ("");
			}
			default:
				return QVariant ();
			}
		}
		else if (role == LeechCraft::RoleControls)
			return QVariant::fromValue<QToolBar*> (Toolbar_);
		else if (role == CustomDataRoles::RoleJobHolderRow)
			return QVariant::fromValue<JobHolderRow> (JobHolderRow::DownloadProgress);
		else if (role == ProcessState::Done)
			return TaskAt (index.row ()).Task_->GetDone ();
		else if (role == ProcessState::Total)
			return TaskAt (index.row ()).Task_->GetTotal ();
		else if (role == ProcessState::TaskFlags)
			return QVariant::fromValue (TaskAt (index.row ()).Parameters_);
		else
			return QVariant ();
	}

	Qt::ItemFlags Core::flags (const QModelIndex&) const
	{
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	}

	bool Core::hasChildren (const QModelIndex& index) const
	{
		return !index.isValid ();
	}

	QVariant Core::headerData (int column, Qt::Orientation orient, int role) const
	{
		if (orient == Qt::Horizontal && role == Qt::DisplayRole)
			return Headers_.at (column);
		else
			return QVariant ();
	}

	QModelIndex Core::index (int row, int column, const QModelIndex& parent) const
	{
		if (!hasIndex (row, column, parent))
			return QModelIndex ();

		return createIndex (row, column);
	}

	QModelIndex Core::parent (const QModelIndex&) const
	{
		return QModelIndex ();
	}

	int Core::rowCount (const QModelIndex& parent) const
	{
		return parent.isValid () ? 0 : ActiveTasks_.size ();
	}

	void Core::removeTriggered (int i)
	{
		if (i == -1)
		{
			if (!Selected_.isValid ())
				return;
			i = Selected_.row ();
		}

		tasks_t::iterator it = ActiveTasks_.begin ();
		std::advance (it, i);
		Remove (it);
	}

	void Core::removeAllTriggered ()
	{
		while (ActiveTasks_.size ())
			removeTriggered (0);
	}

	void Core::startTriggered (int i)
	{
		if (i == -1)
		{
			if (!Selected_.isValid ())
				return;
			i = Selected_.row ();
		}

		TaskDescr selected = TaskAt (i);
		if (selected.Task_->IsRunning ())
			return;
		if (!selected.File_->open (QIODevice::ReadWrite))
		{
			QString msg = tr ("Could not open file %1: %2")
				.arg (selected.File_->fileName ())
				.arg (selected.File_->error ());
			qWarning () << Q_FUNC_INFO
				<< msg;
			emit error (msg);
			return;
		}
		selected.Task_->Start (selected.File_);
	}

	void Core::stopTriggered (int i)
	{
		if (i == -1)
		{
			if (!Selected_.isValid ())
				return;
			i = Selected_.row ();
		}

		TaskDescr selected = TaskAt (i);
		if (!selected.Task_->IsRunning ())
			return;
		selected.Task_->Stop ();
		selected.File_->close ();
	}

	void Core::startAllTriggered ()
	{
		for (int i = 0, size = ActiveTasks_.size (); i < size; ++i)
			startTriggered (i);
	}

	void Core::stopAllTriggered ()
	{
		for (int i = 0, size = ActiveTasks_.size (); i < size; ++i)
			stopTriggered (i);
	}

	void Core::done (bool err)
	{
		tasks_t::iterator taskdscr = FindTask (sender ());
		if (taskdscr == ActiveTasks_.end ())
			return;

		int id = taskdscr->ID_;
		QString filename = taskdscr->File_->fileName ();
		QString url = taskdscr->Task_->GetURL ();
		if (url.size () > 50)
			url = url.left (50) + "...";
		QString errorStr = taskdscr->Task_->GetErrorString ();
		QStringList tags = taskdscr->Tags_;

		taskdscr->File_->close ();

		bool notifyUser = !(taskdscr->Parameters_ & LeechCraft::DoNotNotifyUser) &&
				!(taskdscr->Parameters_ & LeechCraft::Internal);

		if (notifyUser)
		{
			QString text = err ?
					tr ("Failed downloading %1 (%2).")
						.arg (url)
						.arg (errorStr) :
					tr ("Finished downloading %1 (%2).")
						.arg (filename)
						.arg (url);

			auto e = Util::MakeAN ("CSTP",
					text,
					err ? PCritical_ : PInfo_,
					"org.LeechCraft.CSTP",
					AN::CatDownloads,
					err ? AN::TypeDownloadError : AN::TypeDownloadFinished,
					"org.LC.Plugins.CSTP.DLFinished/" + url, QStringList (QUrl (url).host ()));
			if (!err)
			{
				auto nah = new Util::NotificationActionHandler (e);
				nah->AddFunction (tr ("Handle..."), [this, filename] ()
						{
							auto e = Util::MakeEntity (QUrl::fromLocalFile (filename),
									QString (),
									LeechCraft::FromUserInitiated);
							emit gotEntity (e);
						});
				nah->AddFunction (tr ("Open externally"),
						[filename] ()
						{
							QDesktopServices::openUrl (QUrl::fromLocalFile (filename));
						});
				nah->AddFunction (tr ("Show folder"),
						[filename] () -> void
						{
							const auto& dirPath = QFileInfo (filename).absolutePath ();
							QDesktopServices::openUrl (QUrl::fromLocalFile (dirPath));
						});
			}

			emit gotEntity (e);
		}

		if (!err)
		{
			bool silence = taskdscr->Parameters_ & LeechCraft::DoNotAnnounceEntity;
			auto tp = taskdscr->Parameters_;
			Remove (taskdscr);
			emit taskFinished (id);
			if (!silence)
			{
				tp |= IsDownloaded;
				auto e = Util::MakeEntity (QUrl::fromLocalFile (filename),
						{},
						tp);
				e.Additional_ [" Tags"] = tags;
				emit gotEntity (e);
			}
		}
		else
		{
			qWarning () << Q_FUNC_INFO
					<< "erroneous 'done' for"
					<< id
					<< filename
					<< url
					<< errorStr;
			taskdscr->ErrorFlag_ = true;
			if (notifyUser)
				emit error (errorStr);
			emit taskError (id, IDownload::EUnknown);
			if (taskdscr->Parameters_ & LeechCraft::NotPersistent)
				Remove (taskdscr);
		}
	}

	void Core::updateInterface ()
	{
		tasks_t::const_iterator it = FindTask (sender ());
		if (it == ActiveTasks_.end ())
			return;

		int pos = std::distance<tasks_t::const_iterator>
			(ActiveTasks_.begin (), it);
		emit dataChanged (index (pos, 0), index (pos, columnCount () - 1));
	}

	void Core::writeSettings ()
	{
		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_CSTP");
		settings.beginWriteArray ("ActiveTasks");
		settings.remove ("");
		int taskIndex = 0;
		for (tasks_t::const_iterator i = ActiveTasks_.begin (),
				end = ActiveTasks_.end (); i != end; ++i)
		{
			if (i->Parameters_ & LeechCraft::NotPersistent)
				continue;

			settings.setArrayIndex (taskIndex++);
			settings.setValue ("Task", i->Task_->Serialize ());
			settings.setValue ("Filename", i->File_->fileName ());
			settings.setValue ("Comment", i->Comment_);
			settings.setValue ("ErrorFlag", i->ErrorFlag_);
			settings.setValue ("Tags", i->Tags_);
		}
		SaveScheduled_ = false;
		settings.endArray ();
	}

	void Core::finishedReply (QNetworkReply *rep)
	{
		FinishedReplies_.insert (rep);
	}

	void Core::ReadSettings ()
	{
		QSettings settings (QCoreApplication::organizationName (),
				QCoreApplication::applicationName () + "_CSTP");
		int size = settings.beginReadArray ("ActiveTasks");
		for (int i = 0; i < size; ++i)
		{
			settings.setArrayIndex (i);

			TaskDescr td;

			QByteArray data = settings.value ("Task").toByteArray ();
			td.Task_.reset (new Task ());
			connect (td.Task_.get (),
					SIGNAL (gotEntity (const LeechCraft::Entity&)),
					this,
					SIGNAL (gotEntity (const LeechCraft::Entity&)));
			try
			{
				td.Task_->Deserialize (data);
			}
			catch (const std::runtime_error& e)
			{
				qWarning () << Q_FUNC_INFO << e.what ();
				continue;
			}

			connect (td.Task_.get (),
					SIGNAL (done (bool)),
					this,
					SLOT (done (bool)));
			connect (td.Task_.get (),
					SIGNAL (updateInterface ()),
					this,
					SLOT (updateInterface ()));

			QString filename = settings.value ("Filename").toString ();
			td.File_.reset (new QFile (filename));

			td.Comment_ = settings.value ("Comment").toString ();
			td.ErrorFlag_ = settings.value ("ErrorFlag").toBool ();
			td.Tags_ = settings.value ("Tags").toStringList ();

			ActiveTasks_.push_back (td);
		}
		SaveScheduled_ = false;
		settings.endArray ();
	}

	void Core::ScheduleSave ()
	{
		if (SaveScheduled_)
			return;

		QTimer::singleShot (100, this, SLOT (writeSettings ()));
	}

	Core::tasks_t::const_iterator Core::FindTask (QObject *task) const
	{
		return std::find_if (ActiveTasks_.begin (), ActiveTasks_.end (),
				[task] (const Core::TaskDescr& td) { return task == td.Task_.get (); });
	}

	Core::tasks_t::iterator Core::FindTask (QObject *task)
	{
		return std::find_if (ActiveTasks_.begin (), ActiveTasks_.end (),
				[task] (const Core::TaskDescr& td) { return task == td.Task_.get (); });
	}

	void Core::Remove (tasks_t::iterator it)
	{
		int dst = std::distance (ActiveTasks_.begin (), it);
		int id = it->ID_;
		beginRemoveRows (QModelIndex (), dst, dst);
		ActiveTasks_.erase (it);
		endRemoveRows ();
		CoreProxy_->FreeID (id);

		ScheduleSave ();
	}

	Core::tasks_t::const_reference Core::TaskAt (int pos) const
	{
		tasks_t::const_iterator begin = ActiveTasks_.begin ();
		std::advance (begin, pos);
		return *begin;
	}

	Core::tasks_t::reference Core::TaskAt (int pos)
	{
		tasks_t::iterator begin = ActiveTasks_.begin ();
		std::advance (begin, pos);
		return *begin;
	}
}
}
