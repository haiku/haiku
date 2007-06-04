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

#include "ListenerAdapter.h"

class BMenuBar;
class BMenuItem;
class CommandStack;
class Controller;
class Notifier;
class Playlist;
class PlaylistListView;
class RWLocker;

class PlaylistWindow : public BWindow {
 public:
								PlaylistWindow(BRect frame,
									Playlist* playlist,
									Controller* controller);
	virtual						~PlaylistWindow();

	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* message);

 private:
			void				_CreateMenu(BRect& frame);
			void				_ObjectChanged(const Notifier* object);

			Playlist*			fPlaylist;
			PlaylistListView*	fListView;

			BView*				fTopView;
			BMenuItem*			fUndoMI;
			BMenuItem*			fRedoMI;

			RWLocker*			fLocker;
			CommandStack*		fCommandStack;
			ListenerAdapter		fCommandStackListener;
};

#endif // PLAYLIST_WINDOW_H
