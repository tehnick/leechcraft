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

#include "ipfilterdialog.h"
#include "core.h"
#include "banpeersdialog.h"

namespace LeechCraft
{
namespace BitTorrent
{
	const int BlockRole = 101;

	IPFilterDialog::IPFilterDialog (QWidget *parent)
	: QDialog (parent)
	{
		Ui_.setupUi (this);

		QMap<Core::BanRange_t, bool> filter = Core::Instance ()->GetFilter ();
		QList<Core::BanRange_t> keys = filter.keys ();
		Q_FOREACH (Core::BanRange_t key, keys)
		{
			QTreeWidgetItem *item = new QTreeWidgetItem (Ui_.Tree_);
			item->setText (0, key.first);
			item->setText (1, key.second);
			bool block = filter [key];
			item->setText (2, block ?
					tr ("block") :
					tr ("allow"));
			item->setData (2, BlockRole, block);
		}

		on_Tree__currentItemChanged (0);
	}

	QList<QPair<Core::BanRange_t, bool>> IPFilterDialog::GetFilter () const
	{
		QList<QPair<Core::BanRange_t, bool>> result;
		for (int i = 0, size = Ui_.Tree_->topLevelItemCount (); i < size; ++i)
		{
			QTreeWidgetItem *item = Ui_.Tree_->topLevelItem (i);
			result << qMakePair (qMakePair (item->text (0), item->text (1)),
					item->data (2, BlockRole).toBool ());
		}
		return result;
	}

	void IPFilterDialog::on_Tree__currentItemChanged (QTreeWidgetItem *current)
	{
		Ui_.Modify_->setEnabled (current);
		Ui_.Remove_->setEnabled (current);
	}

	void IPFilterDialog::on_Tree__itemClicked (QTreeWidgetItem *item, int column)
	{
		if (column != 2)
			return;

		bool block = !item->data (2, BlockRole).toBool ();
		item->setData (2, BlockRole, block);
		item->setText (2, block ?
				tr ("block") :
				tr ("allow"));
	}

	void IPFilterDialog::on_Add__released ()
	{
		BanPeersDialog dia;
		if (dia.exec () != QDialog::Accepted)
			return;

		QString start = dia.GetStart ();
		QString end = dia.GetEnd ();
		if (start.isEmpty () ||
				end.isEmpty ())
			return;

		QTreeWidgetItem *item = new QTreeWidgetItem (Ui_.Tree_);
		item->setText (0, start);
		item->setText (1, end);
		item->setText (2, tr ("block"));
		item->setData (2, BlockRole, true);
	}

	void IPFilterDialog::on_Modify__released ()
	{
		BanPeersDialog dia;
		QTreeWidgetItem *item = Ui_.Tree_->currentItem ();
		dia.SetIP (item->text (0), item->text (1));
		if (dia.exec () != QDialog::Accepted)
			return;

		QString start = dia.GetStart ();
		QString end = dia.GetEnd ();
		if (start.isEmpty () ||
				end.isEmpty ())
			return;

		item->setText (0, start);
		item->setText (1, end);
	}

	void IPFilterDialog::on_Remove__released ()
	{
		delete Ui_.Tree_->currentItem ();
	}
}
}
