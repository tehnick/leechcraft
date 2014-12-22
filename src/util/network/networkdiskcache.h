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
#include <QNetworkDiskCache>
#include <QMutex>
#include <QHash>
#include "networkconfig.h"

namespace LeechCraft
{
namespace Util
{
	/** @brief A thread-safe garbage-collected network disk cache.
	 *
	 * This class is thread-safe unlike the original QNetworkDiskCache,
	 * thus it can be used from multiple threads simultaneously.
	 *
	 * Also, old cache data is automatically removed from the cache in a
	 * background thread without blocking. The garbage collection can be
	 * also triggered manually via the collectGarbage() slot.
	 *
	 * The garbage is collected until cache takes 90% of its maximum size.
	 *
	 * @ingroup NetworkUtil
	 */
	class UTIL_NETWORK_API NetworkDiskCache : public QNetworkDiskCache
	{
		Q_OBJECT

		qint64 CurrentSize_;

		mutable QMutex InsertRemoveMutex_;

		QHash<QIODevice*, QUrl> PendingDev2Url_;
		QHash<QUrl, QList<QIODevice*>> PendingUrl2Devs_;

		const std::shared_ptr<void> GcGuard_;
	public:
		/** @brief Constructs the new disk cache.
		 *
		 * The cache uses a subdirectory \em subpath in the \em network
		 * directory of the user cache location.
		 *
		 * @param[in] subpath The subpath in cache user location.
		 * @param[in] parent The parent object of this cache.
		 *
		 * @sa GetUserDir(), UserDir::Cache.
		 */
		NetworkDiskCache (const QString& subpath, QObject *parent = 0);

		/** @brief Reimplemented from QNetworkDiskCache.
		 */
		virtual qint64 cacheSize () const;

		/** @brief Reimplemented from QNetworkDiskCache.
		 */
		virtual QIODevice* data (const QUrl& url);

		/** @brief Reimplemented from QNetworkDiskCache.
		 */
		virtual void insert (QIODevice *device);

		/** @brief Reimplemented from QNetworkDiskCache.
		 */
		virtual QNetworkCacheMetaData metaData (const QUrl& url);

		/** @brief Reimplemented from QNetworkDiskCache.
		 */
		virtual QIODevice* prepare (const QNetworkCacheMetaData&);

		/** @brief Reimplemented from QNetworkDiskCache.
		 */
		virtual bool remove (const QUrl& url);

		/** @brief Reimplemented from QNetworkDiskCache.
		 */
		virtual void updateMetaData (const QNetworkCacheMetaData& metaData);
	protected:
		/** @brief Reimplemented from QNetworkDiskCache.
		 */
		virtual qint64 expire ();
	};
}
}

