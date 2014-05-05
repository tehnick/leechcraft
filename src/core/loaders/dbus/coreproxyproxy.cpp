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

#include "coreproxyproxy.h"
#include <QModelIndex>
#include <QDBusReply>

namespace LeechCraft
{
namespace DBus
{
	CoreProxyProxy::CoreProxyProxy (const QString& service, const QDBusObjectPath& path)
	: Proxy_ { service, path.path () }
	{
	}

	QNetworkAccessManager* CoreProxyProxy::GetNetworkAccessManager () const
	{
		return nullptr;
	}

	IShortcutProxy* CoreProxyProxy::GetShortcutProxy () const
	{
		return nullptr;
	}

	QModelIndex CoreProxyProxy::MapToSource (const QModelIndex&) const
	{
		return {};
	}

	Util::BaseSettingsManager* CoreProxyProxy::GetSettingsManager () const
	{
		return nullptr;
	}

	IIconThemeManager* CoreProxyProxy::GetIconThemeManager () const
	{
		return nullptr;
	}

	IColorThemeManager* CoreProxyProxy::GetColorThemeManager () const
	{
		return nullptr;
	}

	IRootWindowsManager* CoreProxyProxy::GetRootWindowsManager () const
	{
		return nullptr;
	}

	ITagsManager* CoreProxyProxy::GetTagsManager () const
	{
		return nullptr;
	}

	QStringList CoreProxyProxy::GetSearchCategories () const
	{
		QDBusReply<QStringList> reply { Proxy_.call ("GetSearchCategories") };
		return reply.value ();
	}

	int CoreProxyProxy::GetID ()
	{
		QDBusReply<int> reply { Proxy_.call ("GetID") };
		return reply.value ();
	}

	void CoreProxyProxy::FreeID (int id)
	{
		Proxy_.call ("FreeID", id);
	}

	IPluginsManager* CoreProxyProxy::GetPluginsManager () const
	{
		return nullptr;
	}

	IEntityManager* CoreProxyProxy::GetEntityManager () const
	{
		return nullptr;
	}

	QString CoreProxyProxy::GetVersion () const
	{
		QDBusReply<QString> reply { Proxy_.call ("GetVersion") };
		return reply.value ();
	}

	void CoreProxyProxy::RegisterSkinnable (QAction*)
	{
	}

	bool CoreProxyProxy::IsShuttingDown ()
	{
		QDBusReply<bool> reply { Proxy_.call ("IsShuttingDown") };
		return reply.value ();
	}
}
}
