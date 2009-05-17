/*
 * Copyright 2009 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "PlaylistItem.h"


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


PlaylistItem::PlaylistItem()
{
}


PlaylistItem::~PlaylistItem()
{
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

