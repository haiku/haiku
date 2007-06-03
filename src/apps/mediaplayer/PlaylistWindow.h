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

class Controller;
class Playlist;

class PlaylistWindow : public BWindow {
 public:
								PlaylistWindow(BRect frame,
									Playlist* playlist,
									Controller* controller);
	virtual						~PlaylistWindow();

	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* message);

 private:
			BView*				fTopView;
};

#endif // PLAYLIST_WINDOW_H
