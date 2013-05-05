/*
 * Copyright 2007-2009 Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PLAYLIST_OBSERVER_H
#define PLAYLIST_OBSERVER_H

#include "AbstractLOAdapter.h"
#include "Playlist.h"

enum {
	MSG_PLAYLIST_ITEM_ADDED				= 'plia',
	MSG_PLAYLIST_ITEM_REMOVED			= 'plir',
	MSG_PLAYLIST_ITEMS_SORTED			= 'plis',
	MSG_PLAYLIST_CURRENT_ITEM_CHANGED	= 'plcc',
	MSG_PLAYLIST_IMPORT_FAILED			= 'plif'
};

class PlaylistObserver : public Playlist::Listener, public AbstractLOAdapter {
public:
								PlaylistObserver(BHandler* target);
	virtual						~PlaylistObserver();

	virtual	void				ItemAdded(PlaylistItem* item, int32 index);
	virtual	void				ItemRemoved(int32 index);

	virtual	void				ItemsSorted();

	virtual	void				CurrentItemChanged(int32 newIndex, bool play);

	virtual	void				ImportFailed();
};

#endif // PLAYLIST_OBSERVER_H
