/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2013  Vladislav Tyulbashev
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

#include "vlcwidget.h"
#include "vlcplayer.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QPaintEvent>
#include <QFileDialog>
#include <QTimer>
#include <QDebug>

namespace LeechCraft
{
namespace vlc
{
	VlcWidget::VlcWidget (QObject* parent): QWidget ()
	{
		parent_ = parent;
		ui = new Ui::VlcInteface;
		ui->setupUi (this);
		scrollBar_ = new VlcScrollBar (ui->mainFrame);
		QVBoxLayout *layout = new QVBoxLayout (this);
		layout->addWidget (scrollBar_);
		ui->scrollBarWidget->setLayout (layout);
		scrollBar_->show ();
		vlcPlayer_ = new VlcPlayer (ui->vlcWidget);
		generateToolBar ();
		QTimer *timer = new QTimer;
		timer->setInterval (100);
		timer->start ();
		
		connect (timer,
				SIGNAL (timeout ()),
				 this,
				SLOT (updateIterface ()));
		
		connect (scrollBar_,
				SIGNAL (changePosition (double)),
				 vlcPlayer_,
				SLOT (changePosition (double)));
		
		connect (ui->stop,
				SIGNAL (clicked ()),
				 vlcPlayer_,
				SLOT (stop ()));
				
		connect (ui->play,
				SIGNAL(clicked ()),
				 vlcPlayer_,
				SLOT (play ()));
		
		connect (open_,
				SIGNAL (triggered ()),
				 this,
				SLOT (addFile ()));
	}
	
	VlcWidget::~VlcWidget()
	{
		vlcPlayer_->stop();
		delete vlcPlayer_;
	}


	QObject* VlcWidget::ParentMultiTabs ()
	{
		return parent_;
	}
	
	QToolBar* VlcWidget::GetToolBar () const
	{
		return bar_;
	}
	
	void VlcWidget::Remove () 
	{	
		deleteLater ();
		emit deleteMe (this);
	}
	
	void VlcWidget::addFile()
	{
		QString file = QFileDialog::getOpenFileName(this,
													tr("Open file"),
													tr("Videos (*.mkv *.avi *.mov *.mpg)"));
		if (QFile::exists(file))
			vlcPlayer_->addUrl(file);
	}
	
	void VlcWidget::updateIterface()
	{
		if (vlcPlayer_->nowPlaying())
			ui->play->setText(tr("pause"));
		else
			ui->play->setText(tr("play"));
		
		scrollBar_->setPosition(vlcPlayer_->getPosition());
		scrollBar_->repaint();
	}


	void VlcWidget::paintEvent(QPaintEvent *event)
	{
		QPainter p(this);
		p.drawEllipse(100, 100, 100, 100);
		p.end();
	}

	void VlcWidget::generateToolBar() {
		bar_ = new QToolBar();
		open_ = bar_->addAction(tr("Open"));
		info_ = bar_->addAction(tr("Info"));
	}
	
	TabClassInfo VlcWidget::GetTabClassInfo () const
	{
		return VlcWidget::GetTabInfo ();
	}
	
	TabClassInfo VlcWidget::GetTabInfo () 
	{
		static TabClassInfo main;
		main.Description_ = "Main tab for VLC plugin";
		main.Priority_ = 1;
		main.Icon_ = QIcon ();
		main.VisibleName_ = "Name of main tab of VLC plugin";
		main.Features_ = TabFeature::TFOpenableByRequest;
		main.TabClass_ = "org.LeechCraft.vlc";
		return main;
	};
}
}
