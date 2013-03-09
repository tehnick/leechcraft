/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010-2012  Oleg Linkin
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

#include "ljxmlrpc.h"
#include <QDomDocument>
#include <QtDebug>
#include <QCryptographicHash>
#include <QDateTime>
#include <QXmlQuery>
#include <util/sysinfo.h>
#include "profiletypes.h"
#include "ljfriendentry.h"
#include "utils.h"
#include "xmlsettingsmanager.h"

namespace LeechCraft
{
namespace Blogique
{
namespace Metida
{
	LJXmlRPC::LJXmlRPC (LJAccount *acc, QObject *parent)
	: QObject (parent)
	, Account_ (acc)
	, BitMaskForFriendsOnlyComments_ (0)
	, MaxGetEventsCount_ (50)
	{
	}

	void LJXmlRPC::Validate (const QString& login, const QString& password)
	{
		ApiCallQueue_ << [login, password, this] (const QString& challenge)
				{ ValidateAccountData (login, password, challenge); };
		ApiCallQueue_ << [login, password, this] (const QString& challenge)
				{ RequestFriendsInfo (login, password, challenge); };
		//TODO get communities info via parsing page
		GenerateChallenge ();
	}

	void LJXmlRPC::AddNewFriend (const QString& username,
			const QString& bgcolor, const QString& fgcolor, uint groupId)
	{
		ApiCallQueue_ << [username, bgcolor, fgcolor, groupId, this] (const QString& challenge)
				{ AddNewFriendRequest (username, bgcolor, fgcolor, groupId, challenge); };
		GenerateChallenge ();
	}

	void LJXmlRPC::DeleteFriend (const QString& username)
	{
		ApiCallQueue_ << [username, this] (const QString& challenge)
				{ DeleteFriendRequest (username, challenge); };
		GenerateChallenge ();
	}

	void LJXmlRPC::AddGroup (const QString& name, bool isPublic, int id)
	{
		ApiCallQueue_ << [name, isPublic, id, this] (const QString& challenge)
				{ AddGroupRequest (name, isPublic, id, challenge); };
		GenerateChallenge ();
	}

	void LJXmlRPC::DeleteGroup (int id)
	{
		ApiCallQueue_ << [id, this] (const QString& challenge)
				{ DeleteGroupRequest (id, challenge); };
		GenerateChallenge ();
	}

	void LJXmlRPC::UpdateProfileInfo ()
	{
		Validate (Account_->GetOurLogin (), Account_->GetPassword ());
	}

	void LJXmlRPC::Submit (const LJEvent& event)
	{
		ApiCallQueue_ << [event, this] (const QString& challenge)
				{ PostEventRequest (event, challenge); };
		GenerateChallenge ();
	}

	void LJXmlRPC::BackupEvents ()
	{
		ApiCallQueue_ << [this] (const QString& challenge)
				{ BackupEventsRequest (0, challenge); };
		GenerateChallenge ();
	}

	void LJXmlRPC::GetLastEvents (int count)
	{
		ApiCallQueue_ << [count, this] (const QString& challenge)
				{ GetLastEventsRequest (count, challenge); };
		GenerateChallenge ();
	}

	void LJXmlRPC::GetChangedEvents (const QDateTime& dt)
	{
		ApiCallQueue_ << [dt, this] (const QString& challenge)
				{ GetChangedEventsRequest (dt, challenge); };
		GenerateChallenge ();
	}

	void LJXmlRPC::GetEventsByDate (const QDate& date)
	{
		ApiCallQueue_ << [date, this] (const QString& challenge)
				{ GetEventsByDateRequest (date, challenge); };
		GenerateChallenge ();
	}

	void LJXmlRPC::RemoveEvent (const LJEvent& event)
	{
		ApiCallQueue_ << [event, this] (const QString& challenge)
				{ RemoveEventRequest (event, challenge); };
		GenerateChallenge ();
	}

	void LJXmlRPC::UpdateEvent (const LJEvent& event)
	{
		ApiCallQueue_ << [event, this] (const QString& challenge)
				{ UpdateEventRequest (event, challenge); };
		GenerateChallenge ();
	}

	void LJXmlRPC::RequestStatistics ()
	{
		ApiCallQueue_ << [this] (const QString& challenge)
				{ BlogStatisticsRequest (challenge); };
		GenerateChallenge ();
	}

	void LJXmlRPC::RequestFullInbox ()
	{
		ApiCallQueue_ << [this] (const QString& challenge)
				{ InboxRequest (challenge, true, QDateTime::currentDateTime ()); };
		GenerateChallenge ();
	}

	void LJXmlRPC::RequestLastInbox (const QDateTime& from)
	{
		ApiCallQueue_ << [this, from] (const QString& challenge)
				{ InboxRequest (challenge, false, from); };
		GenerateChallenge ();
	}

	namespace
	{
		QPair<QDomElement, QDomElement> GetStartPart (const QString& name,
				QDomDocument doc)
		{
			QDomElement methodCall = doc.createElement ("methodCall");
			doc.appendChild (methodCall);
			QDomElement methodName = doc.createElement ("methodName");
			methodCall.appendChild (methodName);
			QDomText methodNameText = doc.createTextNode (name);
			methodName.appendChild (methodNameText);
			QDomElement params = doc.createElement ("params");
			methodCall.appendChild (params);
			QDomElement param = doc.createElement ("param");
			params.appendChild (param);
			QDomElement value = doc.createElement ("value");
			param.appendChild (value);
			QDomElement structElem = doc.createElement ("struct");
			value.appendChild (structElem);

			return {methodCall, structElem};
		}

		QDomElement GetSimpleMemberElement (const QString& nameVal,
				const QString& typeVal, const QString& value, QDomDocument doc)
		{
			QDomElement member = doc.createElement ("member");
			QDomElement name = doc.createElement ("name");
			member.appendChild (name);
			QDomText nameText = doc.createTextNode (nameVal);
			name.appendChild (nameText);
			QDomElement valueType = doc.createElement ("value");
			member.appendChild (valueType);
			QDomElement type = doc.createElement (typeVal);
			valueType.appendChild (type);
			QDomText text = doc.createTextNode (value);
			type.appendChild (text);

			return member;
		}

		QPair<QDomElement, QDomElement> GetComplexMemberElement (const QString& nameVal,
				const QString& typeVal, QDomDocument doc)
		{
			QDomElement member = doc.createElement ("member");
			QDomElement name = doc.createElement ("name");
			member.appendChild (name);
			QDomText nameText = doc.createTextNode (nameVal);
			name.appendChild (nameText);
			QDomElement valueType = doc.createElement ("value");
			member.appendChild (valueType);
			QDomElement type = doc.createElement (typeVal);
			valueType.appendChild (type);
			QDomElement dataField;
			if (typeVal == "array")
			{
				dataField = doc.createElement ("data");
				type.appendChild (dataField);
			}
			else if (typeVal == "struct")
				dataField = type;

			return { member, dataField };
		}

		QNetworkRequest CreateNetworkRequest ()
		{
			QNetworkRequest request;
			auto userAgent = "LeechCraft Blogique " +
					Core::Instance ().GetCoreProxy ()->GetVersion ().toUtf8 ();
			request.setUrl (QUrl ("http://www.livejournal.com/interface/xmlrpc"));
			request.setRawHeader ("User-Agent", userAgent);
			request.setHeader (QNetworkRequest::ContentTypeHeader, "text/xml");

			return request;
		}

		QString GetPassword (const QString& password, const QString& challenge)
		{
			const QByteArray passwordHash = QCryptographicHash::hash (password.toUtf8 (),
					QCryptographicHash::Md5).toHex ();
			return QCryptographicHash::hash ((challenge + passwordHash).toUtf8 (),
					QCryptographicHash::Md5).toHex ();
		}

		QDomElement FillServicePart (QDomElement parentElement,
				const QString& login, const QString& password,
				const QString& challenge, QDomDocument document)
		{
			parentElement.appendChild (GetSimpleMemberElement ("auth_method", "string",
					"challenge", document));
			parentElement.appendChild (GetSimpleMemberElement ("auth_challenge", "string",
					challenge, document));
			parentElement.appendChild (GetSimpleMemberElement ("username", "string",
					login, document));
			parentElement.appendChild (GetSimpleMemberElement ("auth_response", "string",
					GetPassword (password, challenge), document));
			parentElement.appendChild (GetSimpleMemberElement ("ver", "int",
					"1", document));

			return parentElement;
		}
	}

	void LJXmlRPC::GenerateChallenge () const
	{
		QDomDocument document ("GenerateChallenge");
		QDomElement methodCall = document.createElement ("methodCall");
		document.appendChild (methodCall);

		QDomElement methodName = document.createElement ("methodName");
		methodCall.appendChild (methodName);
		QDomText methodNameText = document.createTextNode ("LJ.XMLRPC.getchallenge");
		methodName.appendChild (methodNameText);

		QNetworkReply *reply = Core::Instance ().GetCoreProxy ()->
				GetNetworkAccessManager ()->post (CreateNetworkRequest (),
						document.toByteArray ());

		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (handleChallengeReplyFinished ()));
		connect (reply,
				SIGNAL (error (QNetworkReply::NetworkError)),
				this,
				SLOT (handleNetworkError (QNetworkReply::NetworkError)));
	}

	void LJXmlRPC::ValidateAccountData (const QString& login,
			const QString& password, const QString& challenge)
	{
		QDomDocument document ("ValidateRequest");
		auto result = GetStartPart ("LJ.XMLRPC.login", document);
		document.appendChild (result.first);

		auto element = FillServicePart (result.second,
				login, password, challenge, document);
		element.appendChild (GetSimpleMemberElement ("clientversion", "string",
				Util::SysInfo::GetOSName () +
					"-LeechCraft Blogique: " +
					Core::Instance ().GetCoreProxy ()->GetVersion (),
				document));
		element.appendChild (GetSimpleMemberElement ("getmoods", "int",
				"0", document));
		element.appendChild (GetSimpleMemberElement ("getmenus", "int",
				"0", document));
		element.appendChild (GetSimpleMemberElement ("getpickws", "int",
				"1", document));
		element.appendChild (GetSimpleMemberElement ("getpickwurls", "int",
				"1", document));
		element.appendChild (GetSimpleMemberElement ("getcaps", "int",
				"1", document));

		QNetworkReply *reply = Core::Instance ().GetCoreProxy ()->
				GetNetworkAccessManager ()->post (CreateNetworkRequest (),
						document.toByteArray ());

		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (handleValidateReplyFinished ()));
		connect (reply,
				SIGNAL (error (QNetworkReply::NetworkError)),
				this,
				SLOT (handleNetworkError (QNetworkReply::NetworkError)));
	}

	void LJXmlRPC::RequestFriendsInfo (const QString& login,
			const QString& password, const QString& challenge)
	{
		QDomDocument document ("GetFriendsInfo");
		auto result = GetStartPart ("LJ.XMLRPC.getfriends", document);
		document.appendChild (result.first);
		auto element = FillServicePart (result.second,
				login, password, challenge, document);
		element.appendChild (GetSimpleMemberElement ("includebdays", "boolean",
				"1", document));
		element.appendChild (GetSimpleMemberElement ("includefriendof", "boolean",
				"1", document));

		QNetworkReply *reply = Core::Instance ().GetCoreProxy ()->
				GetNetworkAccessManager ()->post (CreateNetworkRequest (),
						document.toByteArray ());

		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (handleRequestFriendsInfoFinished ()));
		connect (reply,
				SIGNAL (error (QNetworkReply::NetworkError)),
				this,
				SLOT (handleNetworkError (QNetworkReply::NetworkError)));
	}

	void LJXmlRPC::AddNewFriendRequest (const QString& username,
			const QString& bgcolor, const QString& fgcolor,
			int groupId, const QString& challenge)
	{
		QDomDocument document ("AddNewFriendRequest");
		auto result = GetStartPart ("LJ.XMLRPC.editfriends", document);
		document.appendChild (result.first);

		auto element = FillServicePart (result.second, Account_->GetOurLogin (),
				Account_->GetPassword (), challenge, document);

		auto array = GetComplexMemberElement ("add", "array", document);
 		element.appendChild (array.first);
		auto structField = document.createElement ("struct");
		array.second.appendChild (structField);

		structField.appendChild (GetSimpleMemberElement ("username", "string",
				username, document));
		if (!fgcolor.isEmpty ())
			structField.appendChild (GetSimpleMemberElement ("fgcolor", "string",
					fgcolor, document));
		if (!bgcolor.isEmpty ())
			structField.appendChild (GetSimpleMemberElement ("bgcolor", "string",
					bgcolor, document));
		structField.appendChild (GetSimpleMemberElement ("groupmask", "int",
				QString::number (groupId), document));

		QNetworkReply *reply = Core::Instance ().GetCoreProxy ()->
				GetNetworkAccessManager ()->post (CreateNetworkRequest (),
						document.toByteArray ());

		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (handleAddNewFriendReplyFinished ()));
		connect (reply,
				SIGNAL (error (QNetworkReply::NetworkError)),
				this,
				SLOT (handleNetworkError (QNetworkReply::NetworkError)));
	}

	void LJXmlRPC::DeleteFriendRequest (const QString& username, const QString& challenge)
	{
		QDomDocument document ("DeleteFriendRequest");
		auto result = GetStartPart ("LJ.XMLRPC.editfriends", document);
		document.appendChild (result.first);
		auto element = FillServicePart (result.second, Account_->GetOurLogin (),
				Account_->GetPassword (), challenge, document);

		auto array = GetComplexMemberElement ("delete", "array", document);
		element.appendChild (array.first);
		QDomElement valueField = document.createElement ("value");
		array.second.appendChild (valueField);
		QDomElement valueType = document.createElement ("string");
		valueField.appendChild (valueType);
		QDomText text = document.createTextNode (username);
		valueType.appendChild (text);

		QNetworkReply *reply = Core::Instance ().GetCoreProxy ()->
				GetNetworkAccessManager ()->post (CreateNetworkRequest (),
						document.toByteArray ());

		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (handleReplyWithProfileUpdate ()));
		connect (reply,
				SIGNAL (error (QNetworkReply::NetworkError)),
				this,
				SLOT (handleNetworkError (QNetworkReply::NetworkError)));
	}

	void LJXmlRPC::AddGroupRequest (const QString& name, bool isPublic, int id,
			const QString& challenge)
	{
		QDomDocument document ("AddNewFriendRequest");
		auto result = GetStartPart ("LJ.XMLRPC.editfriendgroups", document);
		document.appendChild (result.first);
		auto element = FillServicePart (result.second, Account_->GetOurLogin (),
				Account_->GetPassword (), challenge, document);

		auto data = GetComplexMemberElement ("set", "struct", document);
		element.appendChild (data.first);
		auto subStruct = GetComplexMemberElement (QString::number (id), "struct", document);
		data.second.appendChild (subStruct.first);
		subStruct.second.appendChild (GetSimpleMemberElement ("name", "string",
				name, document));
		subStruct.second.appendChild (GetSimpleMemberElement ("public", "boolean",
				 isPublic ? "1" : "0", document));

		QNetworkReply *reply = Core::Instance ().GetCoreProxy ()->
				GetNetworkAccessManager ()->post (CreateNetworkRequest (),
						document.toByteArray ());

		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (handleReplyWithProfileUpdate ()));
		connect (reply,
				SIGNAL (error (QNetworkReply::NetworkError)),
				this,
				SLOT (handleNetworkError (QNetworkReply::NetworkError)));
	}

	void LJXmlRPC::DeleteGroupRequest (int id, const QString& challenge)
	{
		QDomDocument document ("DeleteGroupRequest");
		auto result = GetStartPart ("LJ.XMLRPC.editfriendgroups", document);
		document.appendChild (result.first);
		auto element = FillServicePart (result.second, Account_->GetOurLogin (),
				Account_->GetPassword (), challenge, document);

		auto array = GetComplexMemberElement ("delete", "array", document);
		element.appendChild (array.first);
		QDomElement valueField = document.createElement ("value");
		array.second.appendChild (valueField);
		QDomElement valueType = document.createElement ("int");
		valueField.appendChild (valueType);
		QDomText text = document.createTextNode (QString::number (id));
		valueType.appendChild (text);

		QNetworkReply *reply = Core::Instance ().GetCoreProxy ()->
				GetNetworkAccessManager ()->post (CreateNetworkRequest (),
						  document.toByteArray ());

		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (handleReplyWithProfileUpdate ()));
		connect (reply,
				SIGNAL (error (QNetworkReply::NetworkError)),
				this,
				SLOT (handleNetworkError (QNetworkReply::NetworkError)));
	}

	void LJXmlRPC::PostEventRequest (const LJEvent& event, const QString& challenge)
	{
		QDomDocument document ("PostEventRequest");
		auto result = GetStartPart ("LJ.XMLRPC.postevent", document);
		document.appendChild (result.first);
		auto element = FillServicePart (result.second, Account_->GetOurLogin (),
				Account_->GetPassword (), challenge, document);

		element.appendChild (GetSimpleMemberElement ("event", "string",
				event.Event_, document));
		element.appendChild (GetSimpleMemberElement ("subject", "string",
				event.Subject_, document));
		element.appendChild (GetSimpleMemberElement ("security", "string",
				MetidaUtils::GetStringForAccess (event.Security_), document));
		if (event.Security_ == Access::FriendsOnly)
			element.appendChild (GetSimpleMemberElement ("allowmask", "int",
					QString::number (BitMaskForFriendsOnlyComments_), document));
		else if (event.Security_ == Access::Custom)
			element.appendChild (GetSimpleMemberElement ("allowmask", "int",
					QString::number (event.AllowMask_), document));
			element.appendChild (GetSimpleMemberElement ("year", "int",
				QString::number (event.DateTime_.date ().year ()), document));
		element.appendChild (GetSimpleMemberElement ("mon", "int",
				QString::number (event.DateTime_.date ().month ()), document));
		element.appendChild (GetSimpleMemberElement ("day", "int",
				QString::number (event.DateTime_.date ().day ()), document));
		element.appendChild (GetSimpleMemberElement ("hour", "int",
				QString::number (event.DateTime_.time ().hour ()), document));
		element.appendChild (GetSimpleMemberElement ("min", "int",
				QString::number (event.DateTime_.time ().minute ()), document));
		element.appendChild (GetSimpleMemberElement ("usejournal", "string",
				Account_->GetOurLogin (), document));

		auto propsStruct = GetComplexMemberElement ("props", "struct", document);
		element.appendChild (propsStruct.first);
		propsStruct.second.appendChild (GetSimpleMemberElement ("current_location",
				"string", event.Props_.CurrentLocation_, document));
		if (event.Props_.CurrentMoodId_ == -1)
			propsStruct.second.appendChild (GetSimpleMemberElement ("current_mood",
					"string", event.Props_.CurrentMood_, document));
		else
			propsStruct.second.appendChild (GetSimpleMemberElement ("current_moodid",
					"int", QString::number (event.Props_.CurrentMoodId_), document));
		propsStruct.second.appendChild (GetSimpleMemberElement ("current_music",
				"string", event.Props_.CurrentMusic_, document));
		propsStruct.second.appendChild (GetSimpleMemberElement ("opt_nocomments",
				"boolean", event.Props_.CommentsManagement_ == CommentsManagement::DisableComments ?
					"1" :
					"0",
				document));
		propsStruct.second.appendChild (GetSimpleMemberElement ("opt_noemail",
				"boolean", event.Props_.CommentsManagement_ == CommentsManagement::WithoutNotification ?
					"1" :
					"0",
				document));
		QString screening = MetidaUtils::GetStringFromCommentsManagment (event.Props_.CommentsManagement_);
		if (!screening.isEmpty ())
			propsStruct.second.appendChild (GetSimpleMemberElement ("opt_screening",
					"string", screening, document));
		propsStruct.second.appendChild (GetSimpleMemberElement ("adult_content",
				"string",
				MetidaUtils::GetStringForAdultContent (event.Props_.AdultContent_),
				document));
		propsStruct.second.appendChild (GetSimpleMemberElement ("taglist",
				"string", event.Tags_.join (","), document));
		propsStruct.second.appendChild (GetSimpleMemberElement ("useragent",
				"string", "LeechCraft Blogique", document));
		propsStruct.second.appendChild (GetSimpleMemberElement ("picture_keyword",
				"string", event.Props_.PostAvatar_, document));

		QNetworkReply *reply = Core::Instance ().GetCoreProxy ()->
				GetNetworkAccessManager ()->post (CreateNetworkRequest (),
						document.toByteArray ());

		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (handlePostEventReplyFinished ()));
		connect (reply,
				SIGNAL (error (QNetworkReply::NetworkError)),
				this,
				SLOT (handleNetworkError (QNetworkReply::NetworkError)));
	}

	void LJXmlRPC::RemoveEventRequest (const LJEvent& event, const QString& challenge)
	{
		QDomDocument document ("BackupEventsRequest");
		auto result = GetStartPart ("LJ.XMLRPC.editevent", document);
		document.appendChild (result.first);
		auto element = FillServicePart (result.second, Account_->GetOurLogin (),
				Account_->GetPassword (), challenge, document);

		element.appendChild (GetSimpleMemberElement ("itemid", "int",
				QString::number (event.ItemID_), document));
		element.appendChild (GetSimpleMemberElement ("event", "string",
				QString (), document));
		element.appendChild (GetSimpleMemberElement ("usejournal", "string",
				Account_->GetOurLogin (), document));

		QNetworkReply *reply = Core::Instance ().GetCoreProxy ()->
				GetNetworkAccessManager ()->post (CreateNetworkRequest (),
						document.toByteArray ());
		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (handleRemoveEventReplyFinished ()));
		connect (reply,
				SIGNAL (error (QNetworkReply::NetworkError)),
				this,
				SLOT (handleNetworkError (QNetworkReply::NetworkError)));
	}

	void LJXmlRPC::UpdateEventRequest (const LJEvent& event, const QString& challenge)
	{
		QDomDocument document ("EditEventRequest");
		auto result = GetStartPart ("LJ.XMLRPC.editevent", document);
		document.appendChild (result.first);
		auto element = FillServicePart (result.second, Account_->GetOurLogin (),
				Account_->GetPassword (), challenge, document);

		element.appendChild (GetSimpleMemberElement ("itemid", "int",
				QString::number (event.ItemID_), document));
		element.appendChild (GetSimpleMemberElement ("event", "string",
				event.Event_, document));
		element.appendChild (GetSimpleMemberElement ("subject", "string",
				event.Subject_, document));

		element.appendChild (GetSimpleMemberElement ("security", "string",
				MetidaUtils::GetStringForAccess (event.Security_), document));
		if (event.Security_ == Access::FriendsOnly)
			element.appendChild (GetSimpleMemberElement ("allowmask", "int",
					QString::number (BitMaskForFriendsOnlyComments_), document));
		else if (event.Security_ == Access::Custom)
			element.appendChild (GetSimpleMemberElement ("allowmask", "int",
					QString::number (event.AllowMask_), document));
		element.appendChild (GetSimpleMemberElement ("year", "int",
				QString::number (event.DateTime_.date ().year ()), document));
		element.appendChild (GetSimpleMemberElement ("mon", "int",
				QString::number (event.DateTime_.date ().month ()), document));
		element.appendChild (GetSimpleMemberElement ("day", "int",
				QString::number (event.DateTime_.date ().day ()), document));
		element.appendChild (GetSimpleMemberElement ("hour", "int",
				QString::number (event.DateTime_.time ().hour ()), document));
		element.appendChild (GetSimpleMemberElement ("min", "int",
				QString::number (event.DateTime_.time ().minute ()), document));
		element.appendChild (GetSimpleMemberElement ("usejournal", "string",
				Account_->GetOurLogin (), document));

		auto propsStruct = GetComplexMemberElement ("props", "struct", document);
		element.appendChild (propsStruct.first);
		propsStruct.second.appendChild (GetSimpleMemberElement ("current_location",
				"string", event.Props_.CurrentLocation_, document));
		if (!event.Props_.CurrentMood_.isEmpty ())
			propsStruct.second.appendChild (GetSimpleMemberElement ("current_mood",
					"string", event.Props_.CurrentMood_, document));
		else
			propsStruct.second.appendChild (GetSimpleMemberElement ("current_moodid",
					"int", QString::number (event.Props_.CurrentMoodId_), document));
		propsStruct.second.appendChild (GetSimpleMemberElement ("current_music",
				"string", event.Props_.CurrentMusic_, document));
		propsStruct.second.appendChild (GetSimpleMemberElement ("opt_nocomments",
				"boolean", event.Props_.CommentsManagement_ == CommentsManagement::DisableComments ?
					"1" :
					"0",
				document));
		propsStruct.second.appendChild (GetSimpleMemberElement ("opt_noemail",
				"boolean", event.Props_.CommentsManagement_ == CommentsManagement::WithoutNotification ?
					"1" :
					"0",
				document));
		QString screening = MetidaUtils::GetStringFromCommentsManagment (event.Props_.CommentsManagement_);
		if (!screening.isEmpty ())
			propsStruct.second.appendChild (GetSimpleMemberElement ("opt_screening",
					"string", screening, document));
		propsStruct.second.appendChild (GetSimpleMemberElement ("adult_content",
				"string",
				MetidaUtils::GetStringForAdultContent (event.Props_.AdultContent_),
				document));
		propsStruct.second.appendChild (GetSimpleMemberElement ("taglist",
				"string", event.Tags_.join (","), document));
		propsStruct.second.appendChild (GetSimpleMemberElement ("useragent",
				"string", "LeechCraft Blogique", document));
		propsStruct.second.appendChild (GetSimpleMemberElement ("picture_keyword",
				"string", event.Props_.PostAvatar_, document));

		QNetworkReply *reply = Core::Instance ().GetCoreProxy ()->
				GetNetworkAccessManager ()->post (CreateNetworkRequest (),
						document.toByteArray ());

		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (handleUpdateEventReplyFinished ()));
		connect (reply,
				SIGNAL (error (QNetworkReply::NetworkError)),
				this,
				SLOT (handleNetworkError (QNetworkReply::NetworkError)));
	}

	void LJXmlRPC::BackupEventsRequest (int skip, const QString& challenge)
	{
		QDomDocument document ("BackupEventsRequest");
		auto result = GetStartPart ("LJ.XMLRPC.getevents", document);
		document.appendChild (result.first);
		auto element = FillServicePart (result.second, Account_->GetOurLogin (),
				Account_->GetPassword (), challenge, document);

		element.appendChild (GetSimpleMemberElement ("selecttype", "string",
				"before", document));
		element.appendChild (GetSimpleMemberElement ("howmany", "int",
				QString::number (MaxGetEventsCount_), document));
		element.appendChild (GetSimpleMemberElement ("skip", "int",
				QString::number (skip), document));
		element.appendChild (GetSimpleMemberElement ("before", "string",
				QDateTime::currentDateTime ().toString ("yyyy-MM-dd hh:mm:ss"),
				document));
		element.appendChild (GetSimpleMemberElement ("usejournal", "string",
				Account_->GetOurLogin (), document));

		QNetworkReply *reply = Core::Instance ().GetCoreProxy ()->
				GetNetworkAccessManager ()->post (CreateNetworkRequest (),
				document.toByteArray ());
		Reply2Skip_ [reply] = skip;

		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (handleBackupEventsReplyFinished ()));
		connect (reply,
				SIGNAL (error (QNetworkReply::NetworkError)),
				this,
				SLOT (handleNetworkError (QNetworkReply::NetworkError)));
	}

	void LJXmlRPC::GetLastEventsRequest (int count, const QString& challenge)
	{
		QDomDocument document ("GetLastEventsRequest");
		auto result = GetStartPart ("LJ.XMLRPC.getevents", document);
		document.appendChild (result.first);
		auto element = FillServicePart (result.second, Account_->GetOurLogin (),
				Account_->GetPassword (), challenge, document);

		element.appendChild (GetSimpleMemberElement ("selecttype", "string",
				"lastn", document));
		element.appendChild (GetSimpleMemberElement ("howmany", "int",
				QString::number (count), document));
		element.appendChild (GetSimpleMemberElement ("usejournal", "string",
				Account_->GetOurLogin (), document));

		QNetworkReply *reply = Core::Instance ().GetCoreProxy ()->
				GetNetworkAccessManager ()->post (CreateNetworkRequest (),
						document.toByteArray ());

		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (handleGotEventsReplyFinished ()));
		connect (reply,
				SIGNAL (error (QNetworkReply::NetworkError)),
				this,
				SLOT (handleNetworkError (QNetworkReply::NetworkError)));
	}

	void LJXmlRPC::GetChangedEventsRequest (const QDateTime& dt, const QString& challenge)
	{
		QDomDocument document ("GetLastEventsRequest");
		auto result = GetStartPart ("LJ.XMLRPC.getevents", document);
		document.appendChild (result.first);
		auto element = FillServicePart (result.second, Account_->GetOurLogin (),
				Account_->GetPassword (), challenge, document);

		element.appendChild (GetSimpleMemberElement ("selecttype", "string",
				"syncitems", document));
		element.appendChild (GetSimpleMemberElement ("lastsync", "string",
				dt.toString ("yyyy-MM-dd hh:mm:ss"), document));
		element.appendChild (GetSimpleMemberElement ("usejournal", "string",
				Account_->GetOurLogin (), document));

		QNetworkReply *reply = Core::Instance ().GetCoreProxy ()->
				GetNetworkAccessManager ()->post (CreateNetworkRequest (),
						document.toByteArray ());

		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (handleGotEventsReplyFinished ()));
		connect (reply,
				SIGNAL (error (QNetworkReply::NetworkError)),
				this,
				SLOT (handleNetworkError (QNetworkReply::NetworkError)));
	}

	void LJXmlRPC::GetEventsByDateRequest (const QDate& date, const QString& challenge)
	{
		QDomDocument document ("GetLastEventsRequest");
		auto result = GetStartPart ("LJ.XMLRPC.getevents", document);
		document.appendChild (result.first);
		auto element = FillServicePart (result.second, Account_->GetOurLogin (),
				Account_->GetPassword (), challenge, document);

		element.appendChild (GetSimpleMemberElement ("selecttype", "string",
				"day", document));
		element.appendChild (GetSimpleMemberElement ("year", "int",
				QString::number (date.year ()), document));
		element.appendChild (GetSimpleMemberElement ("month", "int",
				QString::number (date.month ()), document));
		element.appendChild (GetSimpleMemberElement ("day", "int",
				QString::number (date.day ()), document));
		element.appendChild (GetSimpleMemberElement ("usejournal", "string",
				Account_->GetOurLogin (), document));

		QNetworkReply *reply = Core::Instance ().GetCoreProxy ()->
				GetNetworkAccessManager ()->post (CreateNetworkRequest (),
						document.toByteArray ());

		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (handleGotEventsReplyFinished ()));
		connect (reply,
				SIGNAL (error (QNetworkReply::NetworkError)),
				this,
				SLOT (handleNetworkError (QNetworkReply::NetworkError)));
	}

	void LJXmlRPC::GetParticularEventRequest (int id, RequestType prt,
			const QString& challenge)
	{
		QDomDocument document ("GetParticularEventsRequest");
		auto result = GetStartPart ("LJ.XMLRPC.getevents", document);
		document.appendChild (result.first);
		auto element = FillServicePart (result.second, Account_->GetOurLogin (),
				Account_->GetPassword (), challenge, document);

		element.appendChild (GetSimpleMemberElement ("selecttype", "string",
				"one", document));
		element.appendChild (GetSimpleMemberElement ("itemid", "int",
				QString::number (id), document));
		element.appendChild (GetSimpleMemberElement ("usejournal", "string",
				Account_->GetOurLogin (), document));

		QNetworkReply *reply = Core::Instance ().GetCoreProxy ()->
				GetNetworkAccessManager ()->post (CreateNetworkRequest (),
						document.toByteArray ());
		Reply2RequestType_ [reply] = prt;

		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (handleGetParticularEventReplyFinished ()));
		connect (reply,
				SIGNAL (error (QNetworkReply::NetworkError)),
				this,
				SLOT (handleNetworkError (QNetworkReply::NetworkError)));
	}

	void LJXmlRPC::BlogStatisticsRequest (const QString& challenge)
	{
		QDomDocument document ("BlogStatisticsRequest");
		auto result = GetStartPart ("LJ.XMLRPC.getdaycounts", document);
		document.appendChild (result.first);
		auto element = FillServicePart (result.second, Account_->GetOurLogin (),
				Account_->GetPassword (), challenge, document);

		element.appendChild (GetSimpleMemberElement ("usejournal", "string",
				Account_->GetOurLogin (), document));

		QNetworkReply *reply = Core::Instance ().GetCoreProxy ()->
				GetNetworkAccessManager ()->post (CreateNetworkRequest (),
						document.toByteArray ());
		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (handleBlogStatisticsReplyFinished ()));
		connect (reply,
				SIGNAL (error (QNetworkReply::NetworkError)),
				this,
				SLOT (handleNetworkError (QNetworkReply::NetworkError)));
	}

	void LJXmlRPC::InboxRequest (const QString& challenge, bool backup,
				const QDateTime& from)
	{
		QDomDocument document ("InboxRequest");
		auto result = GetStartPart ("LJ.XMLRPC.getinbox", document);
		document.appendChild (result.first);
		auto element = FillServicePart (result.second, Account_->GetOurLogin (),
				Account_->GetPassword (), challenge, document);
		element.appendChild (GetSimpleMemberElement (!backup ? "lastsync" : "before", "string",
				QString::number (from.toTime_t ()), document));

		element.appendChild (GetSimpleMemberElement ("extended", "boolean",
				"true", document));

		QNetworkReply *reply = Core::Instance ().GetCoreProxy ()->
				GetNetworkAccessManager ()->post (CreateNetworkRequest (),
						document.toByteArray ());
		Reply2InboxBackup_ [reply] = backup;
		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (handleInboxReplyFinished ()));
		connect (reply,
				SIGNAL (error (QNetworkReply::NetworkError)),
				this,
				SLOT (handleNetworkError (QNetworkReply::NetworkError)));
	}

	void LJXmlRPC::ParseForError (const QByteArray& content)
	{
		QXmlQuery query;
		query.setFocus (content);
		QString errorCode;
		query.setQuery ("/methodResponse/fault/value/struct/member[name='faultCode']/value/int/text()");
		if (!query.evaluateTo (&errorCode))
			return;

		QString errorString;
		query.setQuery ("/methodResponse/fault/value/struct/member[name='faultString']/value/string/text()");
		if (!query.evaluateTo (&errorString))
			return;
		emit error (errorCode.toInt (), errorString);
	}

	namespace
	{
		QVariantList ParseValue (const QDomNode& node);

		LJParserTypes::LJParseProfileEntry ParseMember (const QDomNode& node)
		{
			const auto& memberFields = node.childNodes ();
			const auto& memberNameField = memberFields.at (0);
			const auto& memberValueField = memberFields.at (1);
			QString memberName;
			QVariantList memberValue;
			if (memberNameField.isElement () &&
					memberNameField.toElement ().tagName () == "name")
				memberName = memberNameField.toElement ().text ();

			if (memberValueField.isElement ())
				memberValue = ParseValue (memberValueField);

			return LJParserTypes::LJParseProfileEntry (memberName, memberValue);
		}

		QVariantList ParseValue (const QDomNode& node)
		{
			QVariantList result;
			const auto& valueNode = node.firstChild ();
			const auto& valueElement = valueNode.toElement ();
			QString type = valueElement.tagName ();
			if (type == "string" ||
					type == "int" ||
					type == "i4" ||
					type == "double" ||
					type == "boolean")
				result << valueElement.text ();
			else if (type == "dateTime.iso8601")
				result << QDateTime::fromString (valueElement.text (), Qt::ISODate);
			else if (type == "base64")
				result << QString::fromUtf8 (QByteArray::fromBase64 (valueElement.text ().toUtf8 ()));
			else if (type == "array")
			{
				const auto& arrayElements = valueNode.firstChild ().childNodes ();
				QVariantList array;
				for (int i = 0, count = arrayElements.count (); i < count; ++i)
					array << QVariant::fromValue<QVariantList> (ParseValue (arrayElements.at (i)));

				result << array;
			}
			else if (type == "struct")
			{
				const auto& structMembers = valueNode.childNodes ();
				for (int i = 0, count = structMembers.count (); i < count; ++i)
					result << QVariant::fromValue<LJParserTypes::LJParseProfileEntry> (ParseMember (structMembers.at (i)));
			}

			return result;
		}

		QHash<QString, LJFriendEntry_ptr> CreateFriendEntry (const QString& parentKey, const QVariantList& data)
		{
			QHash<QString, LJFriendEntry_ptr> frHash;
			for (const auto& friendEntry : data)
			{
				LJFriendEntry_ptr fr = std::make_shared<LJFriendEntry> ();
				bool isCommunity = false, personal = false;

				for (const auto& field : friendEntry.toList ())
				{
					auto fieldEntry = field.value<LJParserTypes::LJParseProfileEntry> ();
					if (fieldEntry.Name () == "defaultpicurl")
						fr->SetAvatarUrl (fieldEntry.ValueToUrl ());
					else if (fieldEntry.Name () == "bgcolor")
						fr->SetBGColor (fieldEntry.ValueToString ());
					else if (fieldEntry.Name () == "fgcolor")
						fr->SetFGColor (fieldEntry.ValueToString ());
					else if (fieldEntry.Name () == "groupmask")
						fr->SetGroupMask (fieldEntry.ValueToInt ());
					else if (fieldEntry.Name () == "fullname")
						fr->SetFullName (fieldEntry.ValueToString ());
					else if (fieldEntry.Name () == "username")
						fr->SetUserName (fieldEntry.ValueToString ());
					else if (fieldEntry.Name () == "type")
						isCommunity = (fieldEntry.ValueToString () == "community");
					else if (fieldEntry.Name () == "journaltype")
					{
						isCommunity = (fieldEntry.ValueToString () == "C");
						personal = (fieldEntry.ValueToString () == "P");
					}
					else if (fieldEntry.Name () == "birthday")
						fr->SetBirthday (fieldEntry.ValueToString ());

					if (parentKey == "friends" ||
						parentKey == "added")
						fr->SetMyFriend (true);

					if (parentKey == "friendofs")
						fr->SetFriendOf (true);
				}

				if (!isCommunity ||
					personal)
				{
					if (parentKey == "friendofs" &&
						frHash.contains (fr->GetUserName ()))
						frHash [fr->GetUserName ()]->SetFriendOf (true);
					else if ((parentKey == "friends" ||
						parentKey == "added") &&
						frHash.contains (fr->GetUserName ()))
						frHash [fr->GetUserName ()]->SetMyFriend (true);
					else
						frHash [fr->GetUserName ()] = fr;
				}
			}

			return frHash;
		}

		LJEventProperties CreateLJEventPropetries (QStringList& tags, const QVariantList& data)
		{
			LJEventProperties props;
			for (const auto& prop : data)
			{
				auto propsFieldEntry = prop.value<LJParserTypes::LJParseProfileEntry> ();
				if (propsFieldEntry.Name () == "picture_keyword")
					props.PostAvatar_ = propsFieldEntry.ValueToString ();
				else if (propsFieldEntry.Name () == "opt_screening")
					props.CommentsManagement_ = MetidaUtils::GetCommentsManagmentFromString (propsFieldEntry.ValueToString ());
				else if (propsFieldEntry.Name () == "current_music")
					props.CurrentMusic_ = propsFieldEntry.ValueToString ();
				else if (propsFieldEntry.Name () == "current_mood")
					props.CurrentMood_ = propsFieldEntry.ValueToString ();
				else if (propsFieldEntry.Name () == "current_location")
					props.CurrentLocation_ = propsFieldEntry.ValueToString ();
				else if (propsFieldEntry.Name () == "taglist")
					tags = propsFieldEntry.ValueToString ().split (", ");
				else if (propsFieldEntry.Name () == "adult_content")
					props.AdultContent_ = MetidaUtils::GetAdultContentFromString (propsFieldEntry.ValueToString ());
				else if (propsFieldEntry.Name () == "opt_nocomments")
					props.CommentsManagement_ = MetidaUtils::GetCommentsManagmentFromInt (propsFieldEntry.ValueToInt ());
			}

			return props;
		}

		LJEvent CreateLJEvent (const QString& login, const QVariant& data)
		{
			LJEvent ljEvent;
			for (const auto& field : data.toList ())
			{
				auto fieldEntry = field.value<LJParserTypes::LJParseProfileEntry> ();
				if (fieldEntry.Name () == "itemid")
					ljEvent.ItemID_ = fieldEntry.ValueToLongLong ();
				else if (fieldEntry.Name () == "subject")
					ljEvent.Subject_ = fieldEntry.ValueToString ();
				else if (fieldEntry.Name () == "event")
					ljEvent.Event_ = fieldEntry.ValueToString ();
				else if (fieldEntry.Name () == "ditemid")
					ljEvent.DItemID_ = fieldEntry.ValueToLongLong ();
				else if (fieldEntry.Name () == "eventtime")
					ljEvent.DateTime_ = QDateTime::fromString (fieldEntry.ValueToString (),
															   "yyyy-MM-dd hh:mm:ss");
					else if (fieldEntry.Name () == "props")
					{
						QStringList tags;
						ljEvent.Props_ = CreateLJEventPropetries (tags, fieldEntry.Value ());
						ljEvent.Tags_ = tags;
					}
					else if (fieldEntry.Name () == "url")
						ljEvent.Url_ = QUrl (fieldEntry.ValueToString ());
					else if (fieldEntry.Name () == "anum")
						ljEvent.ANum_ = fieldEntry.ValueToInt ();
					else if (fieldEntry.Name () == "security")
						ljEvent.Security_ = MetidaUtils::GetAccessForString (fieldEntry.ValueToString ());
			}

			return ljEvent;
		}
	}

	void LJXmlRPC::ParseFriends (const QDomDocument& document)
	{
		const auto& firstStructElement = document.elementsByTagName ("struct");

		if (firstStructElement.at (0).isNull ())
			return;

		const auto& members = firstStructElement.at (0).childNodes ();
		QHash<QString, LJFriendEntry_ptr> frHash;
		for (int i = 0, count = members.count (); i < count; ++i)
		{
			const QDomNode& member = members.at (i);
			if (!member.isElement () ||
					member.toElement ().tagName () != "member")
				continue;

			auto res = ParseMember (member);
			if (res.Name () == "friends" ||
					res.Name () == "added" ||
					res.Name () == "friendofs")
				Account_->AddFriends (CreateFriendEntry (res.Name (), res.Value ()).values ());
		}
	}

	QList<LJEvent> LJXmlRPC::ParseFullEvents (const QDomDocument& document)
	{
		QList<LJEvent> events;
		const auto& firstStructElement = document.elementsByTagName ("struct");
		if (firstStructElement.at (0).isNull ())
			return events;

		const auto& members = firstStructElement.at (0).childNodes ();
		for (int i = 0, count = members.count (); i < count; ++i)
		{
			const QDomNode& member = members.at (i);
			if (!member.isElement () ||
				member.toElement ().tagName () != "member")
				continue;

			auto res = ParseMember (member);
			if (res.Name () == "events")
				for (const auto& event : res.Value ())
					events << CreateLJEvent (Account_->GetOurLogin (), event);
		}

		return events;
	}

	QMap<QDate, int> LJXmlRPC::ParseStatistics (const QDomDocument& document)
	{
		QMap<QDate, int> statistics;

		const auto& firstStructElement = document.elementsByTagName ("struct");
		if (firstStructElement.at (0).isNull ())
			return statistics;

		const auto& members = firstStructElement.at (0).childNodes ();
		for (int i = 0, count = members.count (); i < count; ++i)
		{
			const QDomNode& member = members.at (i);
			if (!member.isElement () ||
				member.toElement ().tagName () != "member")
				continue;

			auto res = ParseMember (member);
			if (res.Name () == "daycounts")
				for (const auto& element : res.Value ())
				{
					int count = 0;
					QDate date;
					for (const auto& arrayElem : element.toList ())
					{
						auto entry = arrayElem.value<LJParserTypes::LJParseProfileEntry> ();
						if (entry.Name () == "count")
							count = entry.ValueToInt ();
						else if (entry.Name () == "date")
							date = QDate::fromString (entry.ValueToString (),
									"yyyy-MM-dd");
					}

					statistics [date] = count;
				}
		}

		return statistics;
	}


	namespace
	{
		QByteArray CreateDomDocumentFromReply (QNetworkReply *reply, QDomDocument &document)
		{
			if (!reply)
				return QByteArray ();

			const auto& content = reply->readAll ();
			reply->deleteLater ();
			QString errorMsg;
			int errorLine = -1, errorColumn = -1;
			if (!document.setContent (content, &errorMsg, &errorLine, &errorColumn))
			{
				qWarning () << Q_FUNC_INFO
						<< errorMsg
						<< "in line:"
						<< errorLine
						<< "column:"
						<< errorColumn;
				return QByteArray ();
			}

			return content;
		}
	}

	void LJXmlRPC::handleChallengeReplyFinished ()
	{
		QDomDocument document;
		QByteArray content = CreateDomDocumentFromReply (qobject_cast<QNetworkReply*> (sender ()),
				document);
		if (content.isEmpty ())
			return;

		QXmlQuery query;
		query.setFocus (content);

		QString challenge;
		query.setQuery ("/methodResponse/params/param/value/struct/member[name='challenge']/value/string/text()");
		if (!query.evaluateTo (&challenge))
			return;

		if (!ApiCallQueue_.isEmpty ())
			ApiCallQueue_.dequeue () (challenge.simplified ());
		if (!ApiCallQueue_.isEmpty ())
			GenerateChallenge ();
	}

	namespace
	{
		LJFriendGroup CreateGroup (const QVariantList& data)
		{
			LJFriendGroup group;
			for (const auto& field : data)
			{
				LJParserTypes::LJParseProfileEntry fieldEntry =
						field.value<LJParserTypes::LJParseProfileEntry> ();
				if (fieldEntry.Name () == "public")
					group.Public_ = fieldEntry.ValueToBool ();
				else if (fieldEntry.Name () == "name")
					group.Name_ = fieldEntry.ValueToString ();
				else if (fieldEntry.Name () == "id")
				{
					group.Id_ = fieldEntry.ValueToInt ();
					group.RealId_ = (1 << group.Id_) + 1;
				}
				else if (fieldEntry.Name () == "sortorder")
					group.SortOrder_ = fieldEntry.ValueToInt ();
			}

			return group;
		}

		LJMood CreateMood (const QVariantList& data)
		{
			LJMood mood;
			for (const auto& field : data)
			{
				LJParserTypes::LJParseProfileEntry fieldEntry =
						field.value<LJParserTypes::LJParseProfileEntry> ();
				if (fieldEntry.Name () == "parent")
					mood.Parent_ = fieldEntry.ValueToLongLong ();
				else if (fieldEntry.Name () == "name")
					mood.Name_ = fieldEntry.ValueToString ();
				else if (fieldEntry.Name () == "id")
					mood.Id_ = fieldEntry.ValueToLongLong ();
			}

			return mood;
		}

		struct Id2ProfileField
		{
			QHash<QString, std::function<void (LJProfileData&,
					const LJParserTypes::LJParseProfileEntry&)>> Id2ProfileField_;

			Id2ProfileField ()
			{
				Id2ProfileField_ ["defaultpicurl"] = [] (LJProfileData& profile,
						const LJParserTypes::LJParseProfileEntry& entry)
				{
					profile.AvatarUrl_ = entry.ValueToUrl ();
				};
				Id2ProfileField_ ["friendgroups"] = [] (LJProfileData& profile,
						const LJParserTypes::LJParseProfileEntry& entry)
				{
					for (const auto& friendGroupEntry : entry.Value ())
						profile.FriendGroups_ << CreateGroup (friendGroupEntry.toList ());
				};
				Id2ProfileField_ ["usejournals"] = [] (LJProfileData& profile,
						const LJParserTypes::LJParseProfileEntry& entry)
				{
					for (const auto& val : entry.Value ())
						profile.Communities_ << val.toList ().value (0).toString ();
				};
				Id2ProfileField_ ["fullname"] = [] (LJProfileData& profile,
						const LJParserTypes::LJParseProfileEntry& entry)
				{
					profile.FullName_ = entry.ValueToString ();
				};
				Id2ProfileField_ ["moods"] = [] (LJProfileData& profile,
						const LJParserTypes::LJParseProfileEntry& entry)
				{
					for (const auto& moodEntry : entry.Value ())
						profile.Moods_ << CreateMood (moodEntry.toList ());
				};
				Id2ProfileField_ ["userid"] = [] (LJProfileData& profile,
						const LJParserTypes::LJParseProfileEntry& entry)
				{
					profile.UserId_ = entry.ValueToLongLong ();
				};
				Id2ProfileField_ ["caps"] = [] (LJProfileData& profile,
						const LJParserTypes::LJParseProfileEntry& entry)
				{
					profile.Caps_ = entry.ValueToLongLong ();
				};
				Id2ProfileField_ ["pickws"] = [] (LJProfileData& profile,
						const LJParserTypes::LJParseProfileEntry& entry)
				{
					for (const auto& val : entry.Value ())
						profile.AvatarsID_ << val.toList ().value (0).toString ();
				};
				Id2ProfileField_ ["pickwurls"] = [] (LJProfileData& profile,
						const LJParserTypes::LJParseProfileEntry& entry)
				{
					for (const auto& val : entry.Value ())
						profile.AvatarsUrls_ << QUrl (val.toList ().value (0).toString ());
				};
			}
		};

		LJProfileData ParseProfileInfo (const QDomDocument& document)
		{
			static Id2ProfileField id2field;
			const auto& firstStructElement = document.elementsByTagName ("struct");
			LJProfileData profile;
			if (firstStructElement.at (0).isNull ())
				return LJProfileData ();

			const auto& members = firstStructElement.at (0).childNodes ();
			for (int i = 0, count = members.count (); i < count; ++i)
			{
				const QDomNode& member = members.at (i);
				if (!member.isElement () ||
						member.toElement ().tagName () != "member")
					continue;

				auto res = ParseMember (member);
				if (id2field.Id2ProfileField_.contains (res.Name ()))
					id2field.Id2ProfileField_ [res.Name ()] (profile, res);
			}

			return profile;
		}
	}

	void LJXmlRPC::handleValidateReplyFinished ()
	{
		QDomDocument document;
		QByteArray content = CreateDomDocumentFromReply (qobject_cast<QNetworkReply*> (sender ()),
				document);
		if (content.isEmpty ())
			return;

		if (document.elementsByTagName ("fault").isEmpty ())
		{
			emit profileUpdated (ParseProfileInfo (document));
			emit validatingFinished (true);
			return;
		}
		else
			emit validatingFinished (false);

		ParseForError (content);
	}

	void LJXmlRPC::handleRequestFriendsInfoFinished ()
	{
		QDomDocument document;
		QByteArray content = CreateDomDocumentFromReply (qobject_cast<QNetworkReply*> (sender ()),
				document);
		if (content.isEmpty ())
			return;

		if (document.elementsByTagName ("fault").isEmpty ())
		{
			ParseFriends (document);
			return;
		}

		ParseForError (content);
	}

	void LJXmlRPC::handleAddNewFriendReplyFinished ()
	{
		QDomDocument document;
		QByteArray content = CreateDomDocumentFromReply (qobject_cast<QNetworkReply*> (sender ()),
				document);
		if (content.isEmpty ())
			return;

		if (document.elementsByTagName ("fault").isEmpty ())
		{
			ParseFriends (document);
			return;
		}

		ParseForError (content);
	}

	void LJXmlRPC::handleReplyWithProfileUpdate ()
	{
		QDomDocument document;
		QByteArray content = CreateDomDocumentFromReply (qobject_cast<QNetworkReply*> (sender ()),
				document);
		if (content.isEmpty ())
			return;

		if (document.elementsByTagName ("fault").isEmpty ())
		{
			Account_->updateProfile ();
			return;
		}

		ParseForError (content);
	}

	namespace
	{
		int GetEventItemId (const QDomDocument& document)
		{
			const auto& firstStructElement = document.elementsByTagName ("struct");
			if (firstStructElement.at (0).isNull ())
				return -1;

			const auto& members = firstStructElement.at (0).childNodes ();
			for (int i = 0, count = members.count (); i < count; ++i)
			{
				const QDomNode& member = members.at (i);
				if (!member.isElement () ||
					member.toElement ().tagName () != "member")
					continue;

				auto res = ParseMember (member);
				if (res.Name () == "itemid")
					return res.ValueToInt ();
			}

			return -1;
		}
	}

	void LJXmlRPC::handlePostEventReplyFinished ()
	{
		QDomDocument document;
		QByteArray content = CreateDomDocumentFromReply (qobject_cast<QNetworkReply*> (sender ()),
				document);
		if (content.isEmpty ())
			return;

		if (document.elementsByTagName ("fault").isEmpty ())
		{
			const int id = GetEventItemId (document);
			ApiCallQueue_ << [id, this] (const QString& challenge)
					{ GetParticularEventRequest (id, RequestType::Post, challenge); };
			GenerateChallenge ();
			return;
		}

		ParseForError (content);
	}

	void LJXmlRPC::handleBackupEventsReplyFinished ()
	{
		QNetworkReply *reply = qobject_cast<QNetworkReply*> (sender ());
		if (!reply)
			return;

		QDomDocument document;
		QByteArray content = CreateDomDocumentFromReply (reply, document);
		if (content.isEmpty ())
			return;

		const int skip = Reply2Skip_.take (reply);

		if (document.elementsByTagName ("fault").isEmpty ())
		{
			const auto& eventsList = ParseFullEvents (document);
			emit gotEvents2Backup (eventsList);

			if (!eventsList.isEmpty ())
			{
				ApiCallQueue_ << [skip, this] (const QString& challenge)
						{ BackupEventsRequest (skip + MaxGetEventsCount_, challenge); };
				GenerateChallenge ();
			}
			else
				emit gettingEvents2BackupFinished ();
			return;
		}

		ParseForError (content);
	}

	void LJXmlRPC::handleGotEventsReplyFinished ()
	{
		QDomDocument document;
		QByteArray content = CreateDomDocumentFromReply (qobject_cast<QNetworkReply*> (sender ()),
				document);
		if (content.isEmpty ())
			return;

		if (document.elementsByTagName ("fault").isEmpty ())
		{
			emit gotEvents (ParseFullEvents (document));
			return;
		}

		ParseForError (content);
	}

	void LJXmlRPC::handleRemoveEventReplyFinished ()
	{
		QDomDocument document;
		QByteArray content = CreateDomDocumentFromReply (qobject_cast<QNetworkReply*> (sender ()),
				document);
		if (content.isEmpty ())
			return;

		if (document.elementsByTagName ("fault").isEmpty ())
		{
			emit eventRemoved (GetEventItemId (document));
			return;
		}

		ParseForError (content);
	}

	void LJXmlRPC::handleUpdateEventReplyFinished ()
	{
		QDomDocument document;
		QByteArray content = CreateDomDocumentFromReply (qobject_cast<QNetworkReply*> (sender ()),
				document);
		if (content.isEmpty ())
			return;

		if (document.elementsByTagName ("fault").isEmpty ())
		{
			const int id = GetEventItemId (document);
			ApiCallQueue_ << [id, this] (const QString& challenge)
					{ GetParticularEventRequest (id, RequestType::Update, challenge); };
			GenerateChallenge ();
			return;
		}

		ParseForError (content);
	}

	void LJXmlRPC::handleGetParticularEventReplyFinished ()
	{
		QNetworkReply *reply = qobject_cast<QNetworkReply*> (sender ());
		if (!reply)
			return;

		RequestType rt = Reply2RequestType_.take (reply);
		QDomDocument document;
		QByteArray content = CreateDomDocumentFromReply (reply, document);
		if (content.isEmpty ())
			return;

		if (document.elementsByTagName ("fault").isEmpty ())
		{
			const auto& events = ParseFullEvents (document);
			switch (rt)
			{
			case RequestType::Post:
				emit eventPosted (events);
				break;
			case RequestType::Update:
				emit eventUpdated (events);
				break;
			default:
				emit gotEvents (events);
				break;
			}

			return;
		}

		ParseForError (content);
	}

	void LJXmlRPC::handleBlogStatisticsReplyFinished ()
	{
		QNetworkReply *reply = qobject_cast<QNetworkReply*> (sender ());
		if (!reply)
			return;

		QDomDocument document;
		QByteArray content = CreateDomDocumentFromReply (reply, document);
		if (content.isEmpty ())
			return;

		if (document.elementsByTagName ("fault").isEmpty ())
		{
			emit gotStatistics (ParseStatistics (document));
			return;
		}

		ParseForError (content);
	}

	namespace
	{
		LJInbox::Message* CreateParticularMessage (const QVariant& message,
				int type)
		{
			LJInbox::Message *msg = 0;
			switch (type)
			{
				case LJInbox::JournalNewComment:
					msg = new LJInbox::MessageNewComment;
					break;
				case LJInbox::UserMessageRecvd:
					msg = new LJInbox::MessageRecvd;
					break;
				case LJInbox::UserMessageSent:
					msg = new LJInbox::MessageSent;
					break;
				default:
					msg = new LJInbox::Message;
					break;
			}

			msg->Type_ = type;

			for (const auto& field : message.toList ())
			{
				auto fieldEntry = field.value<LJParserTypes::LJParseProfileEntry> ();
				const QString name = fieldEntry.Name ();
				if (name == "when")
					msg->When_ = QDateTime::fromTime_t (fieldEntry.ValueToLongLong ());
				else if (name == "state")
					msg->State_ = (fieldEntry.ValueToString () == "R") ?
						LJInbox::MessageState::Read :
						LJInbox::MessageState::UnRead;
				else if (name == "qid")
					msg->Id_ = fieldEntry.ValueToInt ();
				else if (name == "typename")
					msg->TypeString_ = fieldEntry.ValueToString ();
				else if (name == "extended")
				{
					for (const auto& extended : fieldEntry.Value ())
					{
						auto extendedEntry = extended.value<LJParserTypes::LJParseProfileEntry> ();
						if (extendedEntry.Name () == "body")
							msg->ExtendedText_ = extendedEntry.ValueToString ();
						else if (extendedEntry.Name () == "subject_raw")
							msg->ExtendedSubject_ = extendedEntry.ValueToString ();
						else if (extendedEntry.Name () == "dtalkid")
							msg->ExternalId_ = extendedEntry.ValueToInt ();
					}
				}
				else if (type == LJInbox::JournalNewComment)
				{
					auto commentMsg = static_cast<LJInbox::MessageNewComment*> (msg);
					if  (name == "journal")
						commentMsg->Journal_ = fieldEntry.ValueToString ();
					else if (name == "action")
						commentMsg->Action_ = fieldEntry.ValueToString ();
					else if (name == "entry")
						commentMsg->Url_ = fieldEntry.ValueToUrl ();
					else if (name == "comment")
						commentMsg->ReplyUrl_ = fieldEntry.ValueToUrl ();
					else if (name == "poster")
						commentMsg->AuthorName_ = fieldEntry.ValueToString ();
				}
				else if (name == "subject")
				{
					switch (type)
					{
					case LJInbox::JournalNewComment:
					{
						auto commentMsg = static_cast<LJInbox::MessageNewComment*> (msg);
						commentMsg->Subject_ = fieldEntry.ValueToString ();
						break;
					}
					case LJInbox::UserMessageRecvd:
					{
						auto recvdMsg = static_cast<LJInbox::MessageRecvd*> (msg);
						recvdMsg->Subject_ = fieldEntry.ValueToString ();
						break;
					}
					case LJInbox::UserMessageSent:
					{
						auto sentMsg = static_cast<LJInbox::MessageSent*> (msg);
						sentMsg->Subject_ = fieldEntry.ValueToString ();
						break;
					}
					default:
						break;
					}
				}
				else if (type == LJInbox::UserMessageRecvd)
				{
					auto recvdMsg = static_cast<LJInbox::MessageRecvd*> (msg);
					if (name == "from")
						recvdMsg->From_ = fieldEntry.ValueToString ();
					else if (name == "msgid")
						recvdMsg->MessageId_ = fieldEntry.ValueToInt ();
					else if (name == "parent" )
						recvdMsg->ParentId_ = fieldEntry.ValueToInt ();
				}
				else if (name == "picture")
				{
					switch (type)
					{
					case LJInbox::UserMessageRecvd:
					{
						auto recvdMsg = static_cast<LJInbox::MessageRecvd*> (msg);
						recvdMsg->PictureUrl_ = fieldEntry.ValueToUrl ();
						break;
					}
					case LJInbox::UserMessageSent:
					{
						auto sentMsg = static_cast<LJInbox::MessageSent*> (msg);
						sentMsg->PictureUrl_ = fieldEntry.ValueToUrl ();
						break;
					}
					default:
						break;
					}
				}
				else if (name == "body")
				{
					switch (type)
					{
					case LJInbox::UserMessageRecvd:
					{
						auto recvdMsg = static_cast<LJInbox::MessageRecvd*> (msg);
						recvdMsg->Body_ = fieldEntry.ValueToString ();
						break;
					}
					case LJInbox::UserMessageSent:
					{
						auto sentMsg = static_cast<LJInbox::MessageSent*> (msg);
						sentMsg->Body_ = fieldEntry.ValueToString ();
						break;
					}
					default:
						break;
					}
				}
				else if (type == LJInbox::UserMessageSent &&
						name == "to")
				{
					auto sentMsg = static_cast<LJInbox::MessageSent*> (msg);
					sentMsg->To_ = fieldEntry.ValueToString ();
				}
			}
			return msg;
		}

		LJInbox::Message* CreateLJMessage (const QVariant& message)
		{
			for (const auto& field : message.toList ())
			{
				auto fieldEntry = field.value<LJParserTypes::LJParseProfileEntry> ();
				if (fieldEntry.Name () == "type")
				{
					switch (fieldEntry.ValueToInt ())
					{
						case LJInbox::JournalNewComment:
							return CreateParticularMessage (message, LJInbox::JournalNewComment);
						case LJInbox::UserMessageRecvd:
							return CreateParticularMessage (message, LJInbox::UserMessageRecvd);
						case LJInbox::UserMessageSent:
							return CreateParticularMessage (message, LJInbox::UserMessageSent);
						default:
							return CreateParticularMessage (message, fieldEntry.ValueToInt ());
					}
				}
			};

			return 0;
		}

		QList<LJInbox::Message*> ParseMessages (QDomDocument document)
		{
			QList<LJInbox::Message*> msgs;
			const auto& firstStructElement = document.elementsByTagName ("struct");
			if (firstStructElement.at (0).isNull ())
				return msgs;

			const auto& members = firstStructElement.at (0).childNodes ();
			for (int i = 0, count = members.count (); i < count; ++i)
			{
				const QDomNode& member = members.at (i);
				if (!member.isElement () ||
						member.toElement ().tagName () != "member")
					continue;

				auto res = ParseMember (member);
				if (res.Name () == "items")
					for (const auto& message : res.Value ())
						msgs << CreateLJMessage (message);
			}

			msgs.removeAll (0);

			return msgs;
		}
	}

	void LJXmlRPC::handleInboxReplyFinished ()
	{
		QNetworkReply *reply = qobject_cast<QNetworkReply*> (sender ());
		if (!reply)
			return;

		QDomDocument document;
		QByteArray content = CreateDomDocumentFromReply (reply, document);
		if (content.isEmpty ())
			return;

		if (document.elementsByTagName ("fault").isEmpty ())
		{
			const auto& msgs = ParseMessages (document);
			emit gotMessages (msgs);
			XmlSettingsManager::Instance ().setProperty ("LastInboxUpdateDate",
					   QDateTime::currentDateTime ());
			if (Reply2InboxBackup_.value (reply, false) && !msgs.isEmpty ())
			{
				QDateTime when = msgs.last ()->When_.addSecs (-1);
				ApiCallQueue_ << [this, when, reply] (const QString& challenge)
						{ InboxRequest (challenge,
								Reply2InboxBackup_.take (reply), when); };
				GenerateChallenge ();
				return;
			}

			emit gotMessagesFinished ();
			XmlSettingsManager::Instance ().setProperty ("FirstInboxDownload",
					true);
			return;
		}

		ParseForError (content);
	}

	void LJXmlRPC::handleNetworkError(QNetworkReply::NetworkError err)
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		if (!reply)
			return;
		reply->deleteLater ();
		emit networkError (err, reply->errorString ());
	}

}
}
}

