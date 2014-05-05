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

#include <QObject>
#include <QDBusInterface>
#include <interfaces/core/icoreproxy.h>

class QDBusObjectPath;

namespace LeechCraft
{
namespace DBus
{
	class CoreProxyProxy : public QObject
						 , public ICoreProxy
	{
		Q_OBJECT
		Q_INTERFACES (ICoreProxy)

		mutable QDBusInterface Proxy_;
	public:
		CoreProxyProxy (const QString& service, const QDBusObjectPath& path);

		QNetworkAccessManager* GetNetworkAccessManager () const;
		IShortcutProxy* GetShortcutProxy () const;
		QModelIndex MapToSource (const QModelIndex&) const;
		Util::BaseSettingsManager* GetSettingsManager () const;
		IIconThemeManager* GetIconThemeManager () const;
		IColorThemeManager* GetColorThemeManager () const;
		IRootWindowsManager* GetRootWindowsManager () const;
		ITagsManager* GetTagsManager () const;
		QStringList GetSearchCategories () const;
		int GetID ();
		void FreeID (int);
		IPluginsManager* GetPluginsManager () const;
		IEntityManager* GetEntityManager () const;
		QString GetVersion() const;
		void RegisterSkinnable (QAction*);
		bool IsShuttingDown();
	};
}
}
