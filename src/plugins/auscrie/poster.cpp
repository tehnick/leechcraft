/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2010  Georg Rudoy
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

#include "poster.h"
#include <QNetworkReply>
#include <QUrl>
#include <QRegExp>
#include <QClipboard>
#include <QApplication>
#include <QtDebug>
#include <interfaces/structures.h>
#include <plugininterface/util.h>
#include "requestbuilder.h"

namespace LeechCraft
{
	namespace Plugins
	{
		namespace Auscrie
		{
			Poster::Poster (const QByteArray& data, const QString& format,
					QNetworkAccessManager *am, QObject *parent)
			: QObject (parent)
			, Reply_ (0)
			{
				QUrl url ("http://imagebin.ca/upload.php");

				RequestBuilder builder;
				builder.AddPair ("t", "file");

				QString name = QString ("screenshot.%1").arg (format.toLower ());
				builder.AddPair ("name", name);
				builder.AddPair ("tags", "leechcraft");
				builder.AddPair ("adult", "f");
				builder.AddFile (format, "f", data);

				QByteArray formed = builder.Build ();

				QNetworkRequest request (url);
				request.setHeader (QNetworkRequest::ContentTypeHeader,
						QString ("multipart/form-data; boundary=" + builder.GetBoundary ()));
				request.setHeader (QNetworkRequest::ContentLengthHeader,
						QString::number (formed.size ()));
				Reply_ = am->post (request, formed);

				connect (Reply_,
						SIGNAL (finished ()),
						this,
						SLOT (handleFinished ()));
				connect (Reply_,
						SIGNAL (error (QNetworkReply::NetworkError)),
						this,
						SLOT (handleError ()));
			}

			void Poster::handleFinished ()
			{
				QString result = Reply_->readAll ();

				QRegExp re ("<p>You can find this at <a href='([^<]+)'>([^<]+)</a></p>");
				if (!re.exactMatch (result))
				{
					emit gotEntity (Util::MakeNotification ("Auscrie", tr ("Page parse failed"), PCritical_));
					return;
				}

				QString pasteUrl = re.cap (1);
				pasteUrl.replace ("html", "jpg").replace ("view", "img");

				QApplication::clipboard ()->setText (pasteUrl, QClipboard::Clipboard);
				QApplication::clipboard ()->setText (pasteUrl, QClipboard::Selection);

				QString text = tr ("Image pasted: %1, the URL was copied to the clipboard")
					.arg (pasteUrl);
				emit gotEntity (Util::MakeNotification ("Auscrie", text, PInfo_));

				deleteLater ();
			}

			void Poster::handleError ()
			{
				qWarning () << Q_FUNC_INFO
					<< Reply_->errorString ();

				QString text = tr ("Upload of screenshot failed: %1")
									.arg (Reply_->errorString ());
				emit gotEntity (Util::MakeNotification ("Auscrie", text, PCritical_));

				deleteLater ();
			}
		};
	};
};

