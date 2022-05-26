/*
 * Copyright 2007-2009, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef PLAYLIST_LIST_VIEW_H
#define PLAYLIST_LIST_VIEW_H

#include <PopUpMenu.h>

#include "ListViews.h"

class CommandStack;
class Controller;
class ControllerObserver;
class Playlist;
class PlaylistItem;
class PlaylistObserver;

class PlaylistListView : public SimpleListView {
public:
								PlaylistListView(BRect frame,
									Playlist* playlist,
									Controller* controller,
									CommandStack* stack);
	virtual						~PlaylistListView();

	// BView interface
	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

	virtual	void				MouseDown(BPoint where);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);

	// SimpleListView interface
	virtual	void				MoveItems(const BList& indices, int32 toIndex);
	virtual	void				CopyItems(const BList& indices, int32 toIndex);
	virtual	void				RemoveItemList(const BList& indices);

	virtual	void				DrawListItem(BView* owner, int32 index,
									BRect frame) const;

	// PlaylistListView
			void				ItemsReceived(const BMessage* message,
									int32 appendIndex);

			void				Randomize();
			void				RemoveSelectionToTrash();
			void				RemoveToTrash(int32 index);
			void				RemoveItemList(const BList& indices,
									bool intoTrash);
	virtual	void				SkipBackward();
	virtual	void				SkipForward();

private:
	class Item;

			void				_Wind(bigtime_t howMuch, int64 frames);
			void				_FullSync();
			void				_AddItem(PlaylistItem* item, int32 index);
			void				_RemoveItem(int32 index);

			void				_SetCurrentPlaylistIndex(int32 index);
			void				_SetPlaybackState(uint32 state);

			void				_AddDropContextMenu();
			uint32				_ShowDropContextMenu(BPoint loc);

			Playlist*			fPlaylist;
			PlaylistObserver*	fPlaylistObserver;

			Controller*			fController;
			ControllerObserver*	fControllerObserver;

			CommandStack*		fCommandStack;

			int32				fCurrentPlaylistIndex;
			uint32				fPlaybackState;

			font_height			fFontHeight;
			Item*				fLastClickedItem;

			BPopUpMenu*			fDropContextMenu;
};

#endif // PLAYLIST_LIST_VIEW_H
