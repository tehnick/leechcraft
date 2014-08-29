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

#pragma once

#include <memory>
#include <QObject>
#include <interfaces/iinfo.h>
#include <interfaces/iplugin2.h>
#include <interfaces/ihavesettings.h>
#include <interfaces/azoth/iprovidecommands.h>

namespace LeechCraft
{
namespace Azoth
{
class IProxyObject;

namespace MuCommands
{
	class Plugin : public QObject
				 , public IInfo
				 , public IPlugin2
				 , public IProvideCommands
	{
		Q_OBJECT
		Q_INTERFACES (IInfo
				IPlugin2
				LeechCraft::Azoth::IProvideCommands)

		LC_PLUGIN_METADATA ("org.LeechCraft.Azoth.MuCommands")

		StaticCommand Names_;
		StaticCommand ListUrls_;
		StaticCommand OpenUrl_;
		StaticCommand FetchUrl_;
		StaticCommand VCard_;
		StaticCommand Version_;
		StaticCommand Time_;
		StaticCommand Disco_;
		StaticCommand ChangeNick_;
		StaticCommand ChangeSubject_;
		StaticCommand LeaveMuc_;
		StaticCommand RejoinMuc_;
		StaticCommand Ping_;
		StaticCommand Last_;
		StaticCommand Invite_;
		StaticCommand Pm_;
		StaticCommand Whois_;
		StaticCommand ListPerms_;
		StaticCommand SetPerm_;
		StaticCommand Kick_;
		StaticCommand Ban_;
		StaticCommand Presence_;
		StaticCommand ChatPresence_;

		ICoreProxy_ptr CoreProxy_;
		IProxyObject *AzothProxy_ = nullptr;
	public:
		void Init (ICoreProxy_ptr);
		void SecondInit ();
		QByteArray GetUniqueID () const;
		void Release ();
		QString GetName () const;
		QString GetInfo () const;
		QIcon GetIcon () const;

		QSet<QByteArray> GetPluginClasses () const;

		StaticCommands_t GetStaticCommands (ICLEntry*);
	public slots:
		void initPlugin (QObject*);
	};
}
}
}
