/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#include "PlaylistWindow.h"

#include <ScrollBar.h>
#include <ScrollView.h>

#include "ListViews.h"
#include "Playlist.h"
#include "PlaylistObserver.h"

class PlaylistListView : public SimpleListView {
 public:
								PlaylistListView(BRect frame,
									Playlist* playlist);
	virtual						~PlaylistListView();

	// BView interface
	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

	// SimpleListView interface
	virtual	void				MoveItems(BList& items, int32 toIndex);
	virtual	void				CopyItems(BList& items, int32 toIndex);
	virtual	void				RemoveItemList(BList& indices);

 private:
			void				_FullSync();
			void				_AddItem(const entry_ref& ref, int32 index);
			void				_RemoveItem(int32 index);

			Playlist*			fPlaylist;
			PlaylistObserver*	fPlaylistObserver;
};


// #pragma mark -


PlaylistListView::PlaylistListView(BRect frame, Playlist* playlist)
	: SimpleListView(frame, "playlist listview", NULL)
	, fPlaylist(playlist)
	, fPlaylistObserver(new PlaylistObserver(this))
{
	fPlaylist->AddListener(fPlaylistObserver);
}


PlaylistListView::~PlaylistListView()
{
	fPlaylist->RemoveListener(fPlaylistObserver);
	delete fPlaylistObserver;
}


void
PlaylistListView::AttachedToWindow()
{
	_FullSync();
	SimpleListView::AttachedToWindow();
}


void
PlaylistListView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_PLAYLIST_REF_ADDED: {
			entry_ref ref;
			int32 index;
			if (message->FindRef("refs", &ref) == B_OK
				&& message->FindInt32("index", &index) == B_OK)
				_AddItem(ref, index);
			break;
		}
		case MSG_PLAYLIST_REF_REMOVED: {
			int32 index;
			if (message->FindInt32("index", &index) == B_OK)
				_RemoveItem(index);
			break;
		}
		case MSG_PLAYLIST_REFS_SORTED:
			_FullSync();
			break;
		default:
			SimpleListView::MessageReceived(message);
			break;
	}
}


void
PlaylistListView::MoveItems(BList& items, int32 toIndex)
{
	// TODO: drag indices instead of items! (avoids IndexOf())
	int32 count = items.CountItems();
	for (int32 i = 0; i < count; i++) {
		BListItem* item = (BListItem*)items.ItemAtFast(i);
		int32 index = IndexOf(item);
		entry_ref ref = fPlaylist->RemoveRef(index);
	}
}


void
PlaylistListView::CopyItems(BList& items, int32 toIndex)
{
}


void
PlaylistListView::RemoveItemList(BList& indices)
{
	int32 count = indices.CountItems();
	for (int32 i = 0; i < count; i++) {
		int32 index = (int32)indices.ItemAtFast(i);
		fPlaylist->RemoveRef(index);
	}
}


void
PlaylistListView::_FullSync()
{
//	BScrollBar* scrollBar = ScrollBar(B_VERTICAL);
//	SetScrollBar();
//
	MakeEmpty();

	int32 count = fPlaylist->CountItems();
	for (int32 i = 0; i < count; i++) {
		entry_ref ref;
		if (fPlaylist->GetRefAt(i, &ref) == B_OK)
			_AddItem(ref, i);
	}
}


void
PlaylistListView::_AddItem(const entry_ref& ref, int32 index)
{
	SimpleItem* item = new SimpleItem(ref.name);
	AddItem(item, index);
}


void
PlaylistListView::_RemoveItem(int32 index)
{
	delete RemoveItem(index);
}


// #pragma mark -


PlaylistWindow::PlaylistWindow(BRect frame, Playlist* playlist)
	: BWindow(frame, "Playlist", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	frame = Bounds();
	frame.right -= B_V_SCROLL_BAR_WIDTH;
	PlaylistListView* listView = new PlaylistListView(frame, playlist);

	fTopView = new BScrollView("playlist scrollview",
		listView, B_FOLLOW_ALL, 0, false, true, B_NO_BORDER);

	AddChild(fTopView);
}


PlaylistWindow::~PlaylistWindow()
{
	// give listeners a chance to detach themselves
	fTopView->RemoveSelf();
	delete fTopView;
}


bool
PlaylistWindow::QuitRequested()
{
	Hide();
	return false;
}

