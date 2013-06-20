/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
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

#include "flatmountableitems.h"
#include <QStandardItemModel>
#include <QtDebug>
#include <interfaces/iremovabledevmanager.h>
#include <util/util.h>

namespace LeechCraft
{
namespace Vrooby
{
	FlatMountableItems::FlatMountableItems (QObject *parent)
	: Util::FlattenFilterModel (parent)
	{
		QHash<int, QByteArray> names;
		names [DeviceRoles::VisibleName] = "devName";
		names [DeviceRoles::DevFile] = "devFile";
		names [DeviceRoles::IsRemovable] = "isRemovable";
		names [DeviceRoles::IsPartition] = "isPartition";
		names [DeviceRoles::IsMountable] = "isMountable";
		names [DeviceRoles::DevID] = "devID";
		names [DeviceRoles::DevPersistentID] = "devPersistentID";
		names [DeviceRoles::AvailableSize] = "availableSize";
		names [DeviceRoles::TotalSize] = "totalSize";
		names [CustomRoles::FormattedTotalSize] = "formattedTotalSize";
		names [CustomRoles::FormattedFreeSpace] = "formattedFreeSpace";
		names [CustomRoles::UsedPercentage] = "usedPercentage";
		names [CustomRoles::MountButtonIcon] = "mountButtonIcon";
		names [CustomRoles::MountedAt] = "mountedAt";
		setRoleNames (names);
	}

	QVariant FlatMountableItems::data (const QModelIndex& index, int role) const
	{
		switch (role)
		{
		case CustomRoles::FormattedTotalSize:
		{
			const auto size = index.data (DeviceRoles::TotalSize).toLongLong ();
			return tr ("total size: %1")
				.arg (Util::MakePrettySize (size));
		}
		case CustomRoles::FormattedFreeSpace:
		{
			const auto size = index.data (DeviceRoles::AvailableSize).toLongLong ();
			return tr ("available size: %1")
				.arg (Util::MakePrettySize (size));
		}
		case CustomRoles::MountButtonIcon:
			return index.data (DeviceRoles::IsMounted).toBool () ?
					"image://ThemeIcons/emblem-unmounted" :
					"image://ThemeIcons/emblem-mounted";
		case CustomRoles::MountedAt:
		{
			const auto& mounts = index.data (DeviceRoles::MountPoints).toStringList ();
			return mounts.isEmpty () ?
					"" :
					tr ("Mounted at %1").arg (mounts.join ("; "));
		}
		case CustomRoles::UsedPercentage:
		{
			const qint64 free = index.data (DeviceRoles::AvailableSize).value<qint64> ();
			if (free < 0)
				return -1;

			const double total = index.data (DeviceRoles::TotalSize).value<qint64> ();
			return (1 - free / total) * 100;
		}
		default:
			return Util::FlattenFilterModel::data (index, role);
		}
	}

	bool FlatMountableItems::IsIndexAccepted (const QModelIndex& child) const
	{
		return child.data (DeviceRoles::IsMountable).toBool ();
	}
}
}
