/*
 * Copyright 2016, Dario Casalinuovo
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef URL_PLAYLIST_ITEM_H
#define URL_PLAYLIST_ITEM_H

#include "PlaylistItem.h"

#include <Url.h>


class UrlPlaylistItem : public PlaylistItem {
public:
								UrlPlaylistItem(BUrl url);
								UrlPlaylistItem(const UrlPlaylistItem& item);
								UrlPlaylistItem(const BMessage* archive);
	virtual						~UrlPlaylistItem();

	virtual	PlaylistItem*		Clone() const;

	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* into,
									bool deep = true) const;

	virtual	status_t			SetAttribute(const Attribute& attribute,
									const BString& string);
	virtual	status_t			GetAttribute(const Attribute& attribute,
									BString& string) const;

	virtual	status_t			SetAttribute(const Attribute& attribute,
									const int32& value);
	virtual	status_t			GetAttribute(const Attribute& attribute,
									int32& value) const;

	virtual	status_t			SetAttribute(const Attribute& attribute,
									const int64& value);
	virtual	status_t			GetAttribute(const Attribute& attribute,
									int64& value) const;

	virtual	status_t			SetAttribute(const Attribute& attribute,
									const float& value);
	virtual	status_t			GetAttribute(const Attribute& attribute,
									float& value) const;

	virtual	BString				LocationURI() const;
	virtual	status_t			GetIcon(BBitmap* bitmap,
									icon_size iconSize) const;

	virtual	status_t			MoveIntoTrash();
	virtual	status_t			RestoreFromTrash();

			BUrl				Url() const;

protected:
	virtual	bigtime_t			_CalculateDuration();

	virtual	TrackSupplier*		_CreateTrackSupplier() const;

private:
			BUrl				fUrl;
			bigtime_t			fDuration;
};

#endif
