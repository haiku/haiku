/*
 * Copyright 2009-2010 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef PLAYLIST_ITEM_H
#define PLAYLIST_ITEM_H


#include <Archivable.h>
#include <List.h>
#include <NodeInfo.h>
#include <Referenceable.h>
#include <String.h>


class BBitmap;
class BMessage;
class TrackSupplier;


class PlaylistItem : public BArchivable, public BReferenceable {
public:
	class Listener {
	public:
						Listener();
		virtual			~Listener();

		virtual	void	ItemChanged(const PlaylistItem* item);
	};

public:
								PlaylistItem();
	virtual						~PlaylistItem();

	virtual	PlaylistItem*		Clone() const = 0;

	// archiving
	virtual	status_t			Archive(BMessage* into,
									bool deep = true) const = 0;

	// attributes
	typedef enum {
		ATTR_STRING_NAME			= 'name',
		ATTR_STRING_KEYWORDS		= 'kwrd',

		ATTR_STRING_ARTIST			= 'arst',
		ATTR_STRING_AUTHOR			= 'auth',
		ATTR_STRING_ALBUM			= 'albm',
		ATTR_STRING_TITLE			= 'titl',
		ATTR_STRING_AUDIO_BITRATE	= 'abtr',
		ATTR_STRING_VIDEO_BITRATE	= 'vbtr',
		ATTR_STRING_DURATION		= 'drtn',

		ATTR_INT32_TRACK			= 'trck',
		ATTR_INT32_YEAR				= 'year',
		ATTR_INT32_RATING			= 'rtng'
	} Attribute;

	virtual	status_t			SetAttribute(const Attribute& attribute,
									const BString& string) = 0;
	virtual	status_t			GetAttribute(const Attribute& attribute,
									BString& string) const = 0;

	virtual	status_t			SetAttribute(const Attribute& attribute,
									const int32& value) = 0;
	virtual	status_t			GetAttribute(const Attribute& attribute,
									int32& value) const = 0;

	virtual	status_t			SetAttribute(const Attribute& attribute,
									const int64& value) = 0;
	virtual	status_t			GetAttribute(const Attribute& attribute,
									int64& value) const = 0;

	// convenience access to attributes
			BString				Name() const;
			BString				Author() const;
			BString				Album() const;
			BString				Title() const;

			int32				TrackNumber() const;

	// methods
	virtual	BString				LocationURI() const = 0;
	virtual	status_t			GetIcon(BBitmap* bitmap,
									icon_size iconSize) const = 0;

	virtual	status_t			MoveIntoTrash() = 0;
	virtual	status_t			RestoreFromTrash() = 0;

	// playback
	virtual	TrackSupplier*		CreateTrackSupplier() const = 0;

			void				SetPlaybackFailed();
			bool				PlaybackFailed() const
									{ return fPlaybackFailed; }

	// listener support
			bool				AddListener(Listener* listener);
			void				RemoveListener(Listener* listener);

protected:
			void				_NotifyListeners() const;

private:
			BList				fListeners;
			bool				fPlaybackFailed;
};

typedef BReference<PlaylistItem> PlaylistItemRef;

#endif // PLAYLIST_ITEM_H
