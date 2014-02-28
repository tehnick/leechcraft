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

#include "carbonsmanager.h"
#include <QDomElement>
#include <QXmppClient.h>

namespace LeechCraft
{
namespace Azoth
{
namespace Xoox
{
	namespace
	{
		const QString NsCarbons { "urn:xmpp:carbons:2" };
	}

	QStringList CarbonsManager::discoveryFeatures () const
	{
		return { NsCarbons };
	}

	bool CarbonsManager::handleStanza (const QDomElement& stanza)
	{
		if (stanza.tagName () == "iq" &&
				stanza.attribute ("id") == LastReqId_)
		{
			LastReqId_.clear ();

			if (stanza.attribute ("type") == "result")
			{
				LastConfirmedState_ = LastReqState_;
				emit stateChanged (LastConfirmedState_);
			}
			else
			{
				QXmppIq iq;
				iq.parse (stanza);
				emit stateChangeError (iq);
			}

			return true;
		}

		return false;
	}

	void CarbonsManager::SetEnabled (bool enabled)
	{
		QXmppIq iq { QXmppIq::Set };

		QXmppElement elem;
		elem.setTagName (enabled ? "enable" : "disable");
		elem.setAttribute ("xmlns", NsCarbons);
		iq.setExtensions ({ elem });

		client ()->sendPacket (iq);

		LastReqId_ = iq.id ();
		LastReqState_ = enabled;
	}

	bool CarbonsManager::IsEnabled () const
	{
		return LastConfirmedState_;
	}
}
}
}