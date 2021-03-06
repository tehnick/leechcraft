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
#include <QMap>
#include <QtPlugin>

class QModelIndex;
class QAbstractItemModel;

namespace Media
{
	class IRadioStation;
	typedef std::shared_ptr<IRadioStation> IRadioStation_ptr;

	/** @brief Describes the type of a radio station.
	 *
	 * @sa IRadioStationProvider
	 */
	enum RadioType
	{
		/** @brief No type (item doesn't correspond to a radio station).
		 */
		None,

		/** @brief Radio of artists similar to a given one.
		 *
		 * When selecting a radio of this type the GUI should present the
		 * user with a dialog where he can enter the desired artist's
		 * name which should be passed to the
		 * IRadioStationProvider::GetRadioStation() method.
		 */
		SimilarArtists,

		/** @brief Radio of a global tag like \em metalcore.
		 *
		 * When selecting a radio of this type the GUI should present the
		 * user with a dialog where he can enter the desired tag name
		 * which should be passed to the
		 * IRadioStationProvider::GetRadioStation() method.
		 */
		GlobalTag,

		/** @brief A predefined radio station like an Icecast stream.
		 */
		Predefined,

		/** @brief A radio station that contains user-addable streams.
		 *
		 * This can be used to implement bookmarks, for example, or a
		 * custom collection.
		 *
		 * A radio station returned for the item with this radio type
		 * from the IRadioStationProvider::GetRadioStation() method
		 * should also implement IModifiableRadioStation.
		 *
		 * @sa IModifiableRadioStation
		 */
		CustomAddableStreams,

		/** @brief A predefined list of single tracks, not a stream.
		 *
		 * Items of this type should provide RadioItemRole::TracksInfos.
		 */
		TracksList,

		/** @brief A single song.
		 *
		 * Items of this type should provide RadioItemRole::TracksInfos.
		 */
		SingleTrack,

		/** @brief A root for a list of SingleTrack or TracksList items.
		 */
		TracksRoot,

		/** @brief An action.
		 *
		 * Items of this type represent an action, like "Sign in to radio"
		 * or something.
		 */
		RadioAction
	};

	/** @brief Custom user roles for the items in the model.
	 *
	 * @sa IRadioStationProvider
	 */
	enum RadioItemRole
	{
		/** @brief The type of this radio station.
		 *
		 * The value should be a member of the RadioType enumeration.
		 */
		ItemType = Qt::UserRole + 1,

		/** @brief The internal ID of the radio.
		 *
		 * The value should be a QString.
		 */
		RadioID,

		/** @brief The tracks list.
		 *
		 * This role should be available for RadioType::SingleTrack and
		 * RadioType::TracksList. The role should return a
		 * <code>QList<Media::AudioInfo></code>.The list should consist
		 * of one element for RadioType::SingleTrack and of all child
		 * songs for RadioType::TracksList.
		 *
		 * The Media::AudioInfo elements in the list should contain all
		 * the available metadata and must have the "URL" element in the
		 * additional map.
		 *
		 * @sa Media::AudioInfo
		 */
		TracksInfos,

		/** @brief Maximum role.
		 */
		MaxRadioRole
	};

	/** @brief Interface for plugins providing radio stations.
	 *
	 * Plugins that provide access to radio stations, either static like
	 * Icecast streams or dynamic like library radio or similar artists
	 * radio in Last.FM should implement this interface.
	 */
	class Q_DECL_EXPORT IRadioStationProvider
	{
	public:
		virtual ~IRadioStationProvider () {}

		/** @brief Returns a radio station for the given item and query.
		 *
		 * The \em item should be the one of returned from the
		 * GetRadioListItems() method or its child. The \em query is only
		 * used for RadioType::SimilarArtists and RadioType::GlobalTag
		 * radio station types, where it is the source artist name and
		 * tag name correspondingly. Otherwise it can be any string and
		 * shouldn't be taken into account.
		 *
		 * @param[in] item The item identifying the radio station.
		 * @param[in] query The additional user input, if applicable.
		 * @return The radio station object.
		 */
		virtual IRadioStation_ptr GetRadioStation (const QModelIndex& item, const QString& query) = 0;

		/** @brief Returns the list of stations provided by this plugin.
		 *
		 * The returned items should have proper text and icon set, as
		 * well as the RadioType in the RadioItemRole::ItemType data
		 * role.
		 *
		 * The returned items will typically have children and could be
		 * filled and refreshed dynamically, so it is suggested to add
		 * them as is to a QStandardItemModel.
		 *
		 * The ownership of the items is \em not passed to the caller,
		 * they still belong to the provider plugin.
		 *
		 * @return The list of root items.
		 */
		virtual QList<QAbstractItemModel*> GetRadioListItems () const = 0;

		/** @brief Refreshes the list of radio items.
		 */
		virtual void RefreshItems (const QList<QModelIndex>&) = 0;
	};
}

Q_DECLARE_INTERFACE (Media::IRadioStationProvider, "org.LeechCraft.Media.IRadioStationProvider/1.0");
