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

#include "eq10bandeffect.h"
#include <QtDebug>
#include <gst/gst.h>
#include "eqconfigurator.h"

namespace LeechCraft
{
namespace LMP
{
namespace Fradj
{
	Eq10BandEffect::Eq10BandEffect (const QByteArray& filterId)
	: FilterId_ { filterId }
	, Equalizer_ { gst_element_factory_make ("equalizer-10bands", nullptr) }
	, Configurator_ { new EqConfigurator { this } }
	{
	}

	QByteArray Eq10BandEffect::GetEffectId () const
	{
		return FilterId_;
	}

	QByteArray Eq10BandEffect::GetInstanceId () const
	{
		return FilterId_;
	}

	IFilterConfigurator* Eq10BandEffect::GetConfigurator () const
	{
		return Configurator_;
	}

	const BandInfos_t& Eq10BandEffect::GetFixedBands () const
	{
		return
		{
			{ 29 },
			{ 59 },
			{ 119 },
			{ 237 },
			{ 474 },
			{ 947 },
			{ 1889 },
			{ 3770 },
			{ 7523 },
			{ 15011 }
		};
	}

	void Eq10BandEffect::SetGains (const QList<double>& gains)
	{
		if (gains.size () != GetFixedBands ().size ())
		{
			qWarning () << Q_FUNC_INFO
					<< "unexpected gains count:"
					<< gains;
			return;
		}

		for (int i = 0; i < gains.size (); ++i)
		{
			const auto& optName = "band" + QByteArray::number (i);
			g_object_set (Equalizer_,
					optName.constData (),
					static_cast<gdouble> (gains.at (i)),
					nullptr);
		}
	}

	GstElement* Eq10BandEffect::GetElement () const
	{
		return Equalizer_;
	}
}
}
}
