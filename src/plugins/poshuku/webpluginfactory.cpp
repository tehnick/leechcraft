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

#include "webpluginfactory.h"
#include <util/xpc/defaulthookproxy.h>
#include "core.h"

namespace LeechCraft
{
namespace Poshuku
{
	WebPluginFactory::WebPluginFactory (QObject *parent)
	: QWebPluginFactory (parent)
	{
		Core::Instance ().GetPluginManager ()->
				RegisterHookable (this);
		Reload ();
	}

	WebPluginFactory::~WebPluginFactory ()
	{
	}

	QObject* WebPluginFactory::create (const QString& mime,
			const QUrl& url,
			const QStringList& args, const QStringList& params) const
	{
		QList<IWebPlugin*> plugins = MIME2Plugin_.values (mime);
		Q_FOREACH (IWebPlugin *plugin, plugins)
		{
			QObject *result = plugin->Create (mime, url, args, params);
			if (result)
				return result;
		}
		return 0;
	}

	QList<QWebPluginFactory::Plugin> WebPluginFactory::plugins () const
	{
		QList<Plugin> result;
		Q_FOREACH (IWebPlugin *plugin, Plugins_)
		{
			try
			{
				result << plugin->Plugin (true);
			}
			catch (...)
			{
				// It's ok to do a plain catch(...) {},
				// plugins refuse to add themselves to the list with this.
			}
		}
		return result;
	}

	void WebPluginFactory::refreshPlugins ()
	{
		Reload ();
		QWebPluginFactory::refreshPlugins ();
	}

	void WebPluginFactory::Reload ()
	{
		Plugins_.clear ();
		MIME2Plugin_.clear ();

		emit hookWebPluginFactoryReload (IHookProxy_ptr (new Util::DefaultHookProxy),
				Plugins_);

		Q_FOREACH (IWebPlugin *plugin, Plugins_)
			Q_FOREACH (const QWebPluginFactory::MimeType mime,
					plugin->Plugin (false).mimeTypes)
				MIME2Plugin_.insertMulti (mime.name, plugin);
	}
}
}
