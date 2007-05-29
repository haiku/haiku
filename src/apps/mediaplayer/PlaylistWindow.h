/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef PLAYLIST_WINDOW_H
#define PLAYLIST_WINDOW_H

#include <Window.h>

class Playlist;

class PlaylistWindow : public BWindow {
 public:
								PlaylistWindow(BRect frame,
									Playlist* playlist);
	virtual						~PlaylistWindow();

	virtual	bool				QuitRequested();

 private:
			BView*				fTopView;
};

#endif // PLAYLIST_WINDOW_H
