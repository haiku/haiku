/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef PLAYLIST_OBSERVER_H
#define PLAYLIST_OBSERVER_H

#include "AbstractLOAdapter.h"
#include "Playlist.h"

enum {
	MSG_PLAYLIST_REF_ADDED				= 'plra',
	MSG_PLAYLIST_REF_REMOVED			= 'plrr',
	MSG_PLAYLIST_REFS_SORTED			= 'plrs',
	MSG_PLAYLIST_CURRENT_REF_CHANGED	= 'plcc'
};

class PlaylistObserver : public Playlist::Listener,
						 public AbstractLOAdapter {
 public:
						PlaylistObserver(BHandler* target);
	virtual				~PlaylistObserver();

	virtual	void		RefAdded(const entry_ref& ref, int32 index);
	virtual	void		RefRemoved(int32 index);

	virtual	void		RefsSorted();

	virtual	void		CurrentRefChanged(int32 newIndex);
};

#endif // PLAYLIST_OBSERVER_H
