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

#include "widthiconprovider.h"
#include <interfaces/core/icoreproxy.h>
#include "qmlconfig.h"

namespace LeechCraft
{
namespace Util
{
	/** @brief Provides icons from the current theme by their FDO name.
	 *
	 * This class is used to provide images from the current LeechCraft
	 * icon theme to QML.
	 *
	 * Its usage is as simple as following. First, you should add it to
	 * a QDeclarativeEngine in C++, for example:
	 * \code
	 * QDeclarativeView *view; // some QML view
	 * auto engine = view->engine ();
	 * engine->addImageProvider ("ThemeIcons", new Util::ThemeImageProvider (proxy));
	 * \endcode
	 * Here proxy is the plugin proxy passed to IInfo::Init() method of
	 * your plugin.
	 *
	 * Then in QML:
	 * \code
	 * Image {
	 *     source: "image://ThemeIcons/edit-delete" + / + width
	 * }
	 * \endcode
	 * Or if there is no need in scaling:
	 * \code
	 * Image {
	 *     source: "image://ThemeIcons/edit-delete"
	 * }
	 * \endcode
	 *
	 * One could also use this with ActionButtons:
	 * \code
	 * ActionButton {
	 *     actionIconURL: "image://ThemeIcons/edit-delete"
	 * }
	 * \endcode
	 * In this case there is no need to add width parameter manually,
	 * ActionButton will take care of it.
	 */
	class UTIL_QML_API ThemeImageProvider : public WidthIconProvider
	{
		ICoreProxy_ptr Proxy_;
	public:
		/** @brief Creates the ThemeImageProvider with the given proxy.
		 *
		 * @param[in] proxy The proxy object passed to IInfo::Init() of
		 * your plugin.
		 */
		ThemeImageProvider (ICoreProxy_ptr proxy);

		/** @brief Returns an icon from the current iconset by its ID.
		 *
		 * Reimplemented from WidthIconProvider::GetIcon().
		 *
		 * @param[in] path The icon path, like
		 * <code>QStringList ("edit-delete")</code>.
		 * @return The icon from the current iconset at the given path,
		 * or an empty icon otherwise.
		 */
		QIcon GetIcon (const QStringList& path);
	};
}
}
