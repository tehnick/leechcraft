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

#include "mailmodel.h"
#include <util/util.h>

namespace LeechCraft
{
namespace Snails
{
	MailModel::MailModel (QObject *parent)
	: QAbstractItemModel { parent }
	, Headers_ { tr ("From"), tr ("Subject"), tr ("Date"), tr ("Size")  }
	, Folder_ { "INBOX" }
	{
	}

	int MailModel::columnCount (const QModelIndex&) const
	{
		return Headers_.size ();
	}

	QVariant MailModel::data (const QModelIndex& index, int role) const
	{
		const auto& msg = Messages_.at (index.row ());
		switch (role)
		{
		case Qt::DisplayRole:
		case Sort:
			break;
		case ID:
			return msg->GetID ();
		case ReadStatus:
			return msg->IsRead ();
		default:
			return {};
		}

		switch (static_cast<Column> (index.column ()))
		{
		case Column::From:
			return GetNiceMail (msg->GetAddress (Message::Address::From));
		case Column::Subject:
			return msg->GetSubject ();
		case Column::Date:
			if (role == Sort)
				return msg->GetDate ();
			else
				return msg->GetDate ().toString ();
		case Column::Size:
			if (role == Sort)
				return msg->GetSize ();
			else
				return Util::MakePrettySize (msg->GetSize ());
		default:
			return {};
		}
	}

	QModelIndex MailModel::index (int row, int column, const QModelIndex& parent) const
	{
		if (parent.isValid ())
			return {};

		return createIndex (row, column);
	}

	QModelIndex MailModel::parent (const QModelIndex&) const
	{
		return {};
	}

	int MailModel::rowCount (const QModelIndex& parent) const
	{
		return parent.isValid () ? 0 : Messages_.size ();
	}

	void MailModel::SetFolder (const QStringList& folder)
	{
		Folder_ = folder;
	}

}
}
