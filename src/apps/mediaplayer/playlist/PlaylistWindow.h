/*
 * Copyright 2007-2010, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus 	<superstippi@gmx.de>
 *		Fredrik Modéen 	<fredrik@modeen.se>
 */
#ifndef PLAYLIST_WINDOW_H
#define PLAYLIST_WINDOW_H


#include <Entry.h>
#include <ObjectList.h>
#include <Window.h>

#include "PlaylistObserver.h"
#include "ListenerAdapter.h"


class BMenuBar;
class BMenuItem;
class CommandStack;
class Controller;
class Notifier;
class PlaylistListView;
class RWLocker;
class BButton;
class BFilePanel;
class BStringView;


enum {
	// file
	M_PLAYLIST_OPEN = 'open',
	M_PLAYLIST_SAVE = 'save',
	M_PLAYLIST_SAVE_AS = 'svas',
	M_PLAYLIST_SAVE_RESULT = 'psrs',

	// edit
	M_PLAYLIST_RANDOMIZE = 'rand',
	M_PLAYLIST_REMOVE = 'rmov',
	M_PLAYLIST_MOVE_TO_TRASH = 'trsh'
};


class PlaylistWindow : public BWindow {
public:
								PlaylistWindow(BRect frame,
									Playlist* playlist,
									Controller* controller);
	virtual						~PlaylistWindow();

	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* message);

private:

	class DurationListener : public PlaylistObserver, public BLooper {
	public:

								DurationListener(PlaylistWindow& parent);
								~DurationListener();

			void				MessageReceived(BMessage* message);

			bigtime_t			TotalDuration();

	private:
			void				_HandleItemAdded(PlaylistItem* item,
									int32 index);
			void				_HandleItemRemoved(int32 index);

			BObjectList<bigtime_t>
								fKnown;
			bigtime_t			fTotalDuration;
			PlaylistWindow&		fParent;
	};

	friend class DurationListener;

			void				_CreateMenu(BRect& frame);
			void				_ObjectChanged(const Notifier* object);
			void				_SavePlaylist(const BMessage* filePanelMessage);
			void				_SavePlaylist(const entry_ref& ref);
			void				_SavePlaylist(BEntry& origEntry,
									BEntry& tempEntry, const char* finalName);
			void				_QueryInitialDurations();
			void				_UpdateTotalDuration(bigtime_t duration);

			Playlist*			fPlaylist;
			PlaylistListView*	fListView;

			BView*				fTopView;
			BMenuItem*			fUndoMI;
			BMenuItem*			fRedoMI;

			RWLocker*			fLocker;
			CommandStack*		fCommandStack;
			ListenerAdapter		fCommandStackListener;

			entry_ref			fSavedPlaylistRef;

			DurationListener*	fDurationListener;
			BStringView*		fTotalDuration;
};


#endif // PLAYLIST_WINDOW_H
