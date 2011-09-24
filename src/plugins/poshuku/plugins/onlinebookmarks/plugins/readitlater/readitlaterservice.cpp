/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2011  Oleg Linkin
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#include "readitlaterservice.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSettings>
#include <QtDebug>
#include "readitlaterauthwidget.h"
#include "readitlaterapi.h"
#include "readitlateraccount.h"

namespace LeechCraft
{
namespace Poshuku
{
namespace OnlineBookmarks
{
namespace ReadItLater
{
	ReadItLaterService::ReadItLaterService (ICoreProxy_ptr proxy)
	: CoreProxy_ (proxy)
	, ReadItLaterApi_ (new ReadItLaterApi)
	{
	}

	void ReadItLaterService::Prepare ()
	{
		RestoreAccounts ();
	}

	IBookmarksService::Features ReadItLaterService::GetFeatures () const
	{
		return FCanRegisterAccount;
	}

	QObject* ReadItLaterService::GetObject ()
	{
		return this;
	}

	QString ReadItLaterService::GetServiceName () const
	{
		return "Read It Later";
	}

	QIcon ReadItLaterService::GetServiceIcon () const
	{
		return QIcon (":/plugins/poshuku/plugins/onlinebookmarks/plugins/readitlater/resources/images/readitlater.ico");
	}

	QWidget* ReadItLaterService::GetAuthWidget ()
	{
		return new ReadItLaterAuthWidget ();
	}

	void ReadItLaterService::CheckAuthData (const QVariantMap& map)
	{
		const QString login = map ["Login"].toString ();
		const QString password = map ["Password"].toString ();
		if (login.isEmpty () || password.isEmpty ())
			return;

		Request req;
		req.Type_ = OTAuth;
		req.Login_ = login;
		req.Password_ = password;

		SendRequest (ReadItLaterApi_->GetAuthUrl (),
				ReadItLaterApi_->GetAuthPayload (login, password),
				req);
	}

	void ReadItLaterService::RegisterAccount(const QVariantMap& map)
	{
		const QString login = map ["Login"].toString ();
		const QString password = map ["Password"].toString ();
		if (login.isEmpty () || password.isEmpty ())
			return;

		Request req;
		req.Type_ = OTRegister;
		req.Login_ = login;
		req.Password_ = password;

		SendRequest (ReadItLaterApi_->GetRegisterUrl (),
				ReadItLaterApi_->GetRegisterPayload (login, password),
				req);
	}

	void ReadItLaterService::SendRequest (const QString& urlSting,
			const QByteArray& payload, Request req)
	{
		QUrl url (urlSting);
		QNetworkRequest request (url);
		QNetworkReply *reply =  CoreProxy_->GetNetworkAccessManager ()->
				post (request, payload);

		Reply2Request_ [reply] = req;

		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (getReplyFinished ()));

		connect (reply,
				SIGNAL (readyRead ()),
				this,
				SLOT (readyReadReply ()));
	}

	void ReadItLaterService::RestoreAccounts ()
	{
		QSettings settings (QSettings::IniFormat, QSettings::UserScope,
				QCoreApplication::organizationName (),
				QCoreApplication::applicationName () +
					"_Poshuku_OnlineBookmarks_ReadItLater_Accounts");

		int size = settings.beginReadArray ("Accounts");
		for (int i = 0; i < size; ++i)
		{
			settings.setArrayIndex (i);
			QByteArray data = settings
					.value ("SerializedData").toByteArray ();

			ReadItLaterAccount *acc = ReadItLaterAccount::Deserialize (data, this);
			if (!acc)
			{
				qWarning () << Q_FUNC_INFO
						<< "unserializable acount"
						<< i;
				continue;
			}

			Accounts_ << acc;
			emit accountAdded (acc);
		}
	}

	void ReadItLaterService::getReplyFinished ()
	{
		QNetworkReply *reply = qobject_cast<QNetworkReply*> (sender ());
		if (!reply)
		{
			qWarning () << Q_FUNC_INFO
					<< sender ()
					<< "isn't a QNetworkReply";
			return;
		}
		reply->deleteLater ();
	}

	void ReadItLaterService::readyReadReply ()
	{
		QNetworkReply *reply = qobject_cast<QNetworkReply*> (sender ());
		if (!reply)
		{
			qWarning () << Q_FUNC_INFO
					<< sender ()
					<< "isn't a QNetworkReply";
			return;
		}

		const QVariant& result = reply->attribute (QNetworkRequest::HttpStatusCodeAttribute);
		switch (result.toInt ())
		{
			case 200:
				if (Reply2Request_ [reply].Type_ == OTAuth ||
						Reply2Request_ [reply].Type_ == OTRegister)
				{
					ReadItLaterAccount *account =
							new ReadItLaterAccount (Reply2Request_ [reply].Login_,
									Reply2Request_ [reply].Password_,
									this);
					Accounts_ << account;
					saveAccounts ();
					emit accountAdded (account);
				}
				break;
			case 400:
				break;
			case 401:
				break;
			case 403:
				break;
			case 503:
				break;
		}
	}

	void ReadItLaterService::saveAccounts () const
	{
		QSettings settings (QSettings::IniFormat, QSettings::UserScope,
				QCoreApplication::organizationName (),
				QCoreApplication::applicationName () +
					"_Poshuku_OnlineBookmarks_ReadItLater_Accounts");

		settings.beginWriteArray ("Accounts");

		for (int i = 0, size = Accounts_.size (); i < size; ++i)
		{
			settings.setArrayIndex (i);
			settings.setValue ("SerializedData",
					Accounts_.at (i)->Serialize ());
		}

		settings.endArray ();
		settings.sync ();
	}

	void ReadItLaterService::removeAccount (QObject* accObj)
	{
		ReadItLaterAccount *account = qobject_cast<ReadItLaterAccount*> (accObj);
		if (Accounts_.removeAll (account))
		{
			accObj->deleteLater ();
			saveAccounts ();
		}
	}

}
}
}
}