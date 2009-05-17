/*
 * Copyright 2009 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef ENTRY_REF_PLAYLIST_ITEM_H
#define ENTRY_REF_PLAYLIST_ITEM_H

#include "PlaylistItem.h"

#include <Entry.h>

class EntryRefPlaylistItem : public PlaylistItem {
public:
								EntryRefPlaylistItem(const entry_ref& ref);
	virtual						~EntryRefPlaylistItem();

	// archiving
//	virtual	status_t			Unarchive(const BMessage* archive);
//	virtual	status_t			Archive(BMessage* into) const;
//
//	virtual	status_t			Unflatten(BDataIO* stream);
//	virtual	status_t			Flatten(BDataIO* stream) const;

	// properties
	virtual	status_t			SetName(const BString& name);
	virtual	status_t			GetName(BString& name) const;

	virtual	status_t			SetTitle(const BString& title);
	virtual	status_t			GetTitle(BString& title) const;

	virtual	status_t			SetAuthor(const BString& author);
	virtual	status_t			GetAuthor(BString& author) const;

	virtual	status_t			SetAlbum(const BString& album);
	virtual	status_t			GetAlbum(BString& album) const;

	virtual	status_t			SetTrackNumber(int32 trackNumber);
	virtual	status_t			GetTrackNumber(int32& trackNumber) const;

	virtual	status_t			SetBitRate(int32 bitRate);
	virtual	status_t			GetBitRate(int32& bitRate) const;

	virtual status_t			GetDuration(bigtime_t& duration) const;

	// methods
	virtual	status_t			MoveIntoTrash();
	virtual	status_t			RestoreFromTrash();

	// playback
	virtual	BMediaFile*			CreateMediaFile() const;

private:
			entry_ref			fRef;
};

#endif // ENTRY_REF_PLAYLIST_ITEM_H
