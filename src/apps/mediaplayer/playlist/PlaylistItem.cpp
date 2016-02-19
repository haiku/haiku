/*
 * Copyright 2009-2010 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "PlaylistItem.h"

#include <stdio.h>

#include <Catalog.h>
#include <Locale.h>

#include "AudioTrackSupplier.h"
#include "TrackSupplier.h"
#include "VideoTrackSupplier.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MediaPlayer-PlaylistItem"


PlaylistItem::Listener::Listener()
{
}

PlaylistItem::Listener::~Listener()
{
}

void PlaylistItem::Listener::ItemChanged(const PlaylistItem* item)
{
}


// #pragma mark -


//#define DEBUG_INSTANCE_COUNT
#ifdef DEBUG_INSTANCE_COUNT
static vint32 sInstanceCount = 0;
#endif


PlaylistItem::PlaylistItem()
	:
	fPlaybackFailed(false)
{
#ifdef DEBUG_INSTANCE_COUNT
	atomic_add(&sInstanceCount, 1);
	printf("%p->PlaylistItem::PlaylistItem() (%ld)\n", this, sInstanceCount);
#endif
}


PlaylistItem::~PlaylistItem()
{
#ifdef DEBUG_INSTANCE_COUNT
	atomic_add(&sInstanceCount, -1);
	printf("%p->PlaylistItem::~PlaylistItem() (%ld)\n", this, sInstanceCount);
#endif
}


BString
PlaylistItem::Name() const
{
	BString name;
	if (GetAttribute(ATTR_STRING_NAME, name) != B_OK)
		name = B_TRANSLATE_CONTEXT("<unnamed>", "PlaylistItem-name");
	return name;
}


BString
PlaylistItem::Author() const
{
	BString author;
	if (GetAttribute(ATTR_STRING_AUTHOR, author) != B_OK)
		author = B_TRANSLATE_CONTEXT("<unknown>", "PlaylistItem-author");
	return author;
}


BString
PlaylistItem::Album() const
{
	BString album;
	if (GetAttribute(ATTR_STRING_ALBUM, album) != B_OK)
		album = B_TRANSLATE_CONTEXT("<unknown>", "PlaylistItem-album");
	return album;
}


BString
PlaylistItem::Title() const
{
	BString title;
	if (GetAttribute(ATTR_STRING_TITLE, title) != B_OK)
		title = B_TRANSLATE_CONTEXT("<untitled>", "PlaylistItem-title");
	return title;
}


int32
PlaylistItem::TrackNumber() const
{
	int32 trackNumber;
	if (GetAttribute(ATTR_INT32_TRACK, trackNumber) != B_OK)
		trackNumber = 0;
	return trackNumber;
}


bigtime_t
PlaylistItem::Duration()
{
	bigtime_t duration;
	if (GetAttribute(ATTR_INT64_DURATION, duration) != B_OK) {
		duration = this->_CalculateDuration();
		SetAttribute(ATTR_INT64_DURATION, duration);
	}

	return duration;
}


void
PlaylistItem::SetPlaybackFailed()
{
	fPlaybackFailed = true;
}


//! You must hold the Playlist lock.
bool
PlaylistItem::AddListener(Listener* listener)
{
	if (listener && !fListeners.HasItem(listener))
		return fListeners.AddItem(listener);
	return false;
}


//! You must hold the Playlist lock.
void
PlaylistItem::RemoveListener(Listener* listener)
{
	fListeners.RemoveItem(listener);
}


void
PlaylistItem::_NotifyListeners() const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		Listener* listener = (Listener*)listeners.ItemAtFast(i);
		listener->ItemChanged(this);
	}
}


bigtime_t PlaylistItem::_CalculateDuration() const
{
	// To be overridden in subclasses with more efficient methods
	TrackSupplier* supplier = CreateTrackSupplier();

	AudioTrackSupplier* au = supplier->CreateAudioTrackForIndex(0);
	VideoTrackSupplier* vi = supplier->CreateVideoTrackForIndex(0);

	bigtime_t duration = max_c(au == NULL ? 0 : au->Duration(),
		vi == NULL ? 0 : vi->Duration());

	delete vi;
	delete au;
	delete supplier;

	return duration;
}

