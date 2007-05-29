/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "PlaylistObserver.h"

#include <Message.h>


PlaylistObserver::PlaylistObserver(BHandler* target)
	: Playlist::Listener()
	, AbstractLOAdapter(target)
{
}


PlaylistObserver::~PlaylistObserver()
{
}


void
PlaylistObserver::RefAdded(const entry_ref& ref, int32 index)
{
	BMessage message(MSG_PLAYLIST_REF_ADDED);
	message.AddRef("refs", &ref);
	message.AddInt32("index", index);

	DeliverMessage(message);
}


void
PlaylistObserver::RefRemoved(int32 index)
{
	BMessage message(MSG_PLAYLIST_REF_REMOVED);
	message.AddInt32("index", index);

	DeliverMessage(message);
}


void
PlaylistObserver::RefsSorted()
{
	BMessage message(MSG_PLAYLIST_REFS_SORTED);

	DeliverMessage(message);
}


void
PlaylistObserver::CurrentRefChanged(int32 newIndex)
{
	BMessage message(MSG_PLAYLIST_CURRENT_REF_CHANGED);
	message.AddInt32("index", newIndex);

	DeliverMessage(message);
}

