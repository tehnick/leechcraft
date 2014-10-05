/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2014  Georg Rudoy
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

#include "servicesmanager.h"
#include <algorithm>
#include <QStringList>
#include <QFileInfo>
#include <QtDebug>
#include "servicebase.h"
#include "pendinguploadbase.h"
#include "bitcheeseservice.h"

namespace LeechCraft
{
namespace Zalil
{
	ServicesManager::ServicesManager (const ICoreProxy_ptr& proxy, QObject *parent)
	: QObject { parent }
	, Proxy_ { proxy }
	{
		Services_ << std::make_shared<BitcheeseService> (proxy, this);
	}

	QStringList ServicesManager::GetNames (const QString& file) const
	{
		const auto fileSize = file.isEmpty () ?
				0 :
				QFileInfo { file }.size ();

		QStringList result;
		for (const auto service : Services_)
			if (service->GetMaxFileSize () > fileSize)
				result << service->GetName ();

		return result;
	}

	PendingUploadBase* ServicesManager::Upload (const QString& file, const QString& svcName)
	{
		const auto pos = std::find_if (Services_.begin (), Services_.end (),
				[&svcName] (const ServiceBase_ptr& service)
					{ return service->GetName () == svcName; });
		if (pos == Services_.end ())
		{
			qWarning () << Q_FUNC_INFO
					<< "cannot find service"
					<< svcName;
			return nullptr;
		}

		const auto pending = (*pos)->UploadFile (file);
		if (!pending)
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to upload"
					<< file
					<< "to"
					<< svcName;
			return nullptr;
		}

		connect (pending,
				SIGNAL (fileUploaded (QString, QUrl)),
				this,
				SIGNAL (fileUploaded (QString, QUrl)));
		return pending;
	}
}
}
