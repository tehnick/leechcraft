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

#include "iconsmanager.h"
#include <QPainter>
#include <util/sll/qtutil.h>
#include <util/sll/prelude.h>
#include <interfaces/an/constants.h>
#include "entryeventsmanager.h"
#include "eventssettingsmanager.h"
#include "xmlsettingsmanager.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Tracolor
{
	IconsManager::IconsManager (EntryEventsManager *evMgr,
			EventsSettingsManager *settingsMgr, QObject *parent)
	: QObject { parent }
	, EvMgr_ { evMgr }
	, SettingsMgr_ { settingsMgr }
	{
		connect (EvMgr_,
				SIGNAL (entryEventRateChanged (QByteArray)),
				this,
				SLOT (handleEntryEventRateChanged (QByteArray)));
		connect (SettingsMgr_,
				SIGNAL (eventsSettingsChanged ()),
				this,
				SLOT (updateCaches ()));

		XmlSettingsManager::Instance ().RegisterObject ("HidingThreshold", this, "updateCaches");
	}

	QList<QIcon> IconsManager::GetIcons (const QByteArray& entryId)
	{
		if (!IconsCache_.contains (entryId))
			RegenCache (entryId);

		return Util::Map (IconsCache_.value (entryId).values (),
				[] (const IconsCacheEntry& item) { return item.Icon_; });
	}

	void IconsManager::RegenCache (const QByteArray& entryId)
	{
		const auto tolerance = 10;

		const auto hideThreshold = XmlSettingsManager::Instance ()
				.property ("HidingThreshold").toInt ();

		QHash<QString, IconsCacheEntry> icons;
		for (const auto& pair : Util::Stlize (SettingsMgr_->GetEnabledEvents ()))
		{
			const auto rate = EvMgr_->GetEntryEventRate (entryId, pair.first.toUtf8 ());
			if (rate < std::numeric_limits<double>::epsilon () * tolerance)
				continue;

			QColor color { pair.second.Color_ };
			color.setAlphaF (rate);
			if (color.alpha () < hideThreshold)
				continue;

			QPixmap px { 22, 22 };
			px.fill (Qt::transparent);
			{
				QPainter p { &px };
				p.setBrush ({ color });
				p.setPen (Qt::NoPen);
				p.drawEllipse ({ px.width () / 2, px.height () / 2 },
						px.width () / 4, px.height () / 4);
			}

			icons.insert (pair.first, { px, color, rate });
		}

		IconsCache_ [entryId] = icons;
	}

	void IconsManager::handleEntryEventRateChanged (const QByteArray& entryId)
	{
		RegenCache (entryId);
		emit iconUpdated (entryId);
	}

	void IconsManager::updateCaches ()
	{
		decltype (IconsCache_) oldCache;
		std::swap (oldCache, IconsCache_);

		for (const auto& pair : Util::Stlize (oldCache))
		{
			RegenCache (pair.first);
#ifdef USE_CPP14
			const auto& oldEntries = pair.second;
			const auto& newEntries = IconsCache_.value (pair.first);

			const auto& stlized = Util::Stlize (newEntries);
			if (oldEntries.size () != newEntries.size () ||
					std::any_of (stlized.begin (), stlized.end (),
							[&oldEntries] (const auto& newEntryPair)
							{
								const auto& oldEntry = oldEntries.value (newEntryPair.first);
								const auto& newEntry = newEntryPair.second;
								return oldEntry.Color_ != newEntry.Color_ ||
										std::abs (oldEntry.Rate_ - newEntry.Rate_) >= 0.05;
							}))
				emit iconUpdated (pair.first);
#endif
		}
	}
}
}
}