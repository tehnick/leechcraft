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

#pragma once

#include <functional>
#include <QObject>

#ifdef ENABLE_MPRIS
#include <qdbuscontext.h>
#endif

#include <interfaces/media/iradiostation.h>
#include "engine/audiosource.h"
#include "mediainfo.h"
#include "sortingcriteria.h"

class QModelIndex;
class QStandardItem;
class QAbstractItemModel;
class QStandardItemModel;

typedef QPair<QString, QString> StringPair_t;

namespace LeechCraft
{
namespace LMP
{
	class SourceObject;
	class Output;
	class Path;
	struct MediaInfo;
	enum class SourceError;
	enum class SourceState;

	class Player : public QObject
#ifdef ENABLE_MPRIS
				 , public QDBusContext
#endif
	{
		Q_OBJECT

		QStandardItemModel *PlaylistModel_;
		SourceObject *Source_;
		Output *Output_;
		Path *Path_;

		QList<AudioSource> CurrentQueue_;
		QHash<AudioSource, QStandardItem*> Items_;
		QHash<QString, QList<QStandardItem*>> AlbumRoots_;

		AudioSource CurrentStopSource_;
		QList<AudioSource> CurrentOneShotQueue_;

		Media::IRadioStation_ptr CurrentStation_;
		QStandardItem *RadioItem_;
		QHash<QUrl, MediaInfo> Url2Info_;

		MediaInfo LastPhononMediaInfo_;

		bool FirstPlaylistRestore_;
		bool IgnoreNextSaves_;
	public:
		enum class PlayMode
		{
			Sequential,
			Shuffle,
			ShuffleAlbums,
			ShuffleArtists,
			RepeatTrack,
			RepeatAlbum,
			RepeatWhole
		};
	private:
		PlayMode PlayMode_;

		struct Sorter
		{
			QList<SortingCriteria> Criteria_;

			Sorter ();
			bool operator() (const MediaInfo&, const MediaInfo&) const;
		} Sorter_;
	public:
		enum Role
		{
			IsCurrent = Qt::UserRole + 1,
			IsStop,
			IsAlbum,
			Source,
			Info,
			AlbumArt,
			AlbumLength,
			OneShotPos
		};

		Player (QObject* = 0);

		QAbstractItemModel* GetPlaylistModel () const;
		SourceObject* GetSourceObject () const;
		Output* GetAudioOutput () const;

		PlayMode GetPlayMode () const;
		void SetPlayMode (PlayMode);

		SourceState GetState () const;

		QList<SortingCriteria> GetSortingCriteria () const;
		void SetSortingCriteria (const QList<SortingCriteria>&);

		void PrepareURLInfo (const QUrl&, const MediaInfo&);
		void Enqueue (const QStringList&, bool = true);
		void Enqueue (const QList<AudioSource>&, bool = true);
		void ReplaceQueue (const QList<AudioSource>&, bool = true);
		QList<AudioSource> GetQueue () const;
		QList<AudioSource> GetIndexSources (const QModelIndex&) const;
		QModelIndex GetSourceIndex (const AudioSource&) const;

		void Dequeue (const QModelIndex&);
		void Dequeue (const QList<AudioSource>&);

		void SetStopAfter (const QModelIndex&);

		void RestorePlayState ();
		void SavePlayState (bool ignoreNext);

		void AddToOneShotQueue (const QModelIndex&);
		void AddToOneShotQueue (const AudioSource&);
		void RemoveFromOneShotQueue (const QModelIndex&);
		void OneShotMoveUp (const QModelIndex&);
		void OneShotMoveDown (const QModelIndex&);
		int GetOneShotQueueSize () const;

		void SetRadioStation (Media::IRadioStation_ptr);

		MediaInfo GetCurrentMediaInfo () const;
		QString GetCurrentAAPath () const;
	private:
		MediaInfo GetMediaInfo (const AudioSource&) const;
		MediaInfo GetPhononMediaInfo () const;
		void AddToPlaylistModel (QList<AudioSource>, bool);

		bool HandleCurrentStop (const AudioSource&);

		void RemoveFromOneShotQueue (const AudioSource&);

		void UnsetRadio ();

		template<typename T>
		AudioSource GetRandomBy (QList<AudioSource>::const_iterator,
				std::function<T (AudioSource)>) const;
		AudioSource GetNextSource (const AudioSource&);
	public slots:
		void play (const QModelIndex&);
		void previousTrack ();
		void nextTrack ();
		void togglePause ();
		void setPause ();
		void stop ();
		void clear ();
		void shufflePlaylist ();
	private slots:
		void handleSorted ();
		void continueAfterSorted (const QList<QPair<AudioSource, MediaInfo>>&);

		void restorePlaylist ();
		void handleStationError (const QString&);
		void handleRadioStream (const QUrl&, const Media::AudioInfo&);
		void handleGotRadioPlaylist (const QString&, const QString&);
		void handleGotAudioInfos (const QList<Media::AudioInfo>&);
		void postPlaylistCleanup (const QString&);
		void handleUpdateSourceQueue ();
		void handlePlaybackFinished ();
		void handleStateChanged ();
		void handleCurrentSourceChanged (const AudioSource&);
		void handleMetadata ();

		void handleSourceError (const QString&, SourceError);

		void refillPlaylist ();
		void setTransitionTime ();
	signals:
		void songChanged (const MediaInfo&);
		void songInfoUpdated (const MediaInfo&);
		void indexChanged (const QModelIndex&);
		void insertedAlbum (const QModelIndex&);

		void playModeChanged (Player::PlayMode);
		void bufferStatusChanged (int);

		void playerAvailable (bool);
	};
}
}
