/*
 * Copyright 2007-2009 Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "PlaylistListView.h"

#include <new>
#include <stdio.h>

#include <Autolock.h>
#include <Catalog.h>
#include <GradientLinear.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <Shape.h>
#include <Window.h>

#include "CommandStack.h"
#include "Controller.h"
#include "ControllerObserver.h"
#include "CopyPLItemsCommand.h"
#include "DurationToString.h"
#include "ImportPLItemsCommand.h"
#include "ListViews.h"
#include "MovePLItemsCommand.h"
#include "PlaybackState.h"
#include "Playlist.h"
#include "PlaylistItem.h"
#include "PlaylistObserver.h"
#include "RandomizePLItemsCommand.h"
#include "RemovePLItemsCommand.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MediaPlayer-PlaylistListView"

using std::nothrow;


enum {
	DISPLAY_NAME	= 0,
	DISPLAY_PATH	= 1,
	M_ADD_SORTED,
	M_ADD_UNSORTED
};


static float
playback_mark_size(const font_height& fh)
{
	return ceilf(fh.ascent * 0.7);
}


static float
text_offset(const font_height& fh)
{
	return ceilf(fh.ascent * 0.8);
}


class PlaylistListView::Item : public SimpleItem,
	public PlaylistItem::Listener {
public:
								Item(PlaylistItem* item);
	virtual						~Item();

			void				Draw(BView* owner, BRect frame,
									const font_height& fh,
									bool tintedLine, uint32 mode,
									bool active,
									uint32 playbackState);

	virtual	void				ItemChanged(const PlaylistItem* item);

#if __GNUC__ == 2
	virtual	void				Draw(BView* owner, BRect frame, uint32 flags);
#else
			using SimpleItem::Draw;
#endif

private:
			PlaylistItemRef		fItem;

};


// #pragma mark -


PlaylistListView::Item::Item(PlaylistItem* item)
	:
	SimpleItem(item->Name().String()),
	fItem(item)
{
	fItem->AddListener(this);
}


PlaylistListView::Item::~Item()
{
	fItem->RemoveListener(this);
}


void
PlaylistListView::Item::Draw(BView* owner, BRect frame, const font_height& fh,
	bool tintedLine, uint32 mode, bool active, uint32 playbackState)
{
	rgb_color color = ui_color(B_LIST_BACKGROUND_COLOR);

	if (IsSelected())
		color = ui_color(B_LIST_SELECTED_BACKGROUND_COLOR);
	if (tintedLine)
		color = tint_color(color, 1.04);
	// background
	owner->SetLowColor(color);
	owner->FillRect(frame, B_SOLID_LOW);
	// label
	if (IsSelected())
		owner->SetHighColor(ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR));
	else
		owner->SetHighColor(ui_color(B_LIST_ITEM_TEXT_COLOR));
	const char* text = Text();
	switch (mode) {
		case DISPLAY_NAME:
			// TODO
			break;
		case DISPLAY_PATH:
			// TODO
			break;
		default:
			break;
	}

	float playbackMarkSize = playback_mark_size(fh);
	float textOffset = text_offset(fh);

	char buffer[64];
	bigtime_t duration = fItem->Duration();
	duration /= 1000000;
	duration_to_string(duration, buffer, sizeof(buffer));

	BString truncatedDuration(buffer);
	owner->TruncateString(&truncatedDuration, B_TRUNCATE_END,
		frame.Width() - playbackMarkSize - textOffset);
	float truncatedWidth = owner->StringWidth(truncatedDuration.String());
	owner->DrawString(truncatedDuration.String(),
		BPoint(frame.right - truncatedWidth,
			floorf(frame.top + frame.bottom + fh.ascent) / 2 - 1));

	BString truncatedString(text);
	owner->TruncateString(&truncatedString, B_TRUNCATE_MIDDLE,
		frame.Width() - playbackMarkSize - textOffset - truncatedWidth);
	owner->DrawString(truncatedString.String(),
		BPoint(frame.left + playbackMarkSize + textOffset,
			floorf(frame.top + frame.bottom + fh.ascent) / 2 - 1));

	// playmark
	if (active) {
		rgb_color green = (rgb_color){ 0, 255, 0, 255 };
		if (playbackState != PLAYBACK_STATE_PLAYING)
			green = tint_color(color, B_DARKEN_1_TINT);

		BRect r(0, 0, playbackMarkSize, playbackMarkSize);
		r.OffsetTo(frame.left + 4,
			ceilf((frame.top + frame.bottom - playbackMarkSize) / 2));

		uint32 flags = owner->Flags();
		owner->SetFlags(flags | B_SUBPIXEL_PRECISE);

		BShape shape;
		shape.MoveTo(r.LeftTop());
		shape.LineTo(r.LeftBottom());
		shape.LineTo(BPoint(r.right, (r.top + r.bottom) / 2));
		shape.Close();

		owner->MovePenTo(B_ORIGIN);
		owner->FillShape(&shape);

		shape.Clear();
		r.InsetBy(1, 1);
		shape.MoveTo(r.LeftTop());
		shape.LineTo(r.LeftBottom());
		shape.LineTo(BPoint(r.right, (r.top + r.bottom) / 2));
		shape.Close();

		BGradientLinear gradient;
		gradient.SetStart(r.LeftTop());
		gradient.SetEnd(r.LeftBottom());
		gradient.AddColor(tint_color(green, B_LIGHTEN_1_TINT), 0);
		gradient.AddColor(tint_color(green, B_DARKEN_1_TINT), 255.0);

		owner->FillShape(&shape, gradient);

		owner->SetFlags(flags);
	}
}


void
PlaylistListView::Item::ItemChanged(const PlaylistItem* item)
{
	// TODO: Invalidate
}


#if __GNUC__ == 2

void
PlaylistListView::Item::Draw(BView* owner, BRect frame, uint32 flags)
{
	SimpleItem::Draw(owner, frame, flags);
}

#endif


// #pragma mark -


PlaylistListView::PlaylistListView(BRect frame, Playlist* playlist,
		Controller* controller, CommandStack* stack)
	:
	SimpleListView(frame, "playlist listview", NULL),

	fPlaylist(playlist),
	fPlaylistObserver(new PlaylistObserver(this)),

	fController(controller),
	fControllerObserver(new ControllerObserver(this,
			OBSERVE_PLAYBACK_STATE_CHANGES)),

	fCommandStack(stack),

	fCurrentPlaylistIndex(-1),
	fPlaybackState(PLAYBACK_STATE_STOPPED),

	fLastClickedItem(NULL)
{
	fPlaylist->AddListener(fPlaylistObserver);
	fController->AddListener(fControllerObserver);
	_AddDropContextMenu();

	SetFlags(Flags() | B_SUBPIXEL_PRECISE);
}


PlaylistListView::~PlaylistListView()
{
	for (int32 i = CountItems() - 1; i >= 0; i--)
		_RemoveItem(i);
	fPlaylist->RemoveListener(fPlaylistObserver);
	delete fPlaylistObserver;
	fController->RemoveListener(fControllerObserver);
	delete fControllerObserver;
}


void
PlaylistListView::AttachedToWindow()
{
	_FullSync();
	SimpleListView::AttachedToWindow();

	GetFontHeight(&fFontHeight);
	MakeFocus(true);
}


void
PlaylistListView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		// PlaylistObserver messages
		case MSG_PLAYLIST_ITEM_ADDED:
		{
			PlaylistItem* item;
			int32 index;
			if (message->FindPointer("item", (void**)&item) == B_OK
				&& message->FindInt32("index", &index) == B_OK)
				_AddItem(item, index);
			break;
		}
		case MSG_PLAYLIST_ITEM_REMOVED:
		{
			int32 index;
			if (message->FindInt32("index", &index) == B_OK)
				_RemoveItem(index);
			break;
		}
		case MSG_PLAYLIST_ITEMS_SORTED:
			_FullSync();
			break;
		case MSG_PLAYLIST_CURRENT_ITEM_CHANGED:
		{
			int32 index;
			if (message->FindInt32("index", &index) == B_OK)
				_SetCurrentPlaylistIndex(index);
			break;
		}
		case MSG_PLAYLIST_IMPORT_FAILED:
			break;

		// ControllerObserver messages
		case MSG_CONTROLLER_PLAYBACK_STATE_CHANGED:
		{
			uint32 state;
			if (message->FindInt32("state", (int32*)&state) == B_OK)
				_SetPlaybackState(state);
			break;
		}

		case B_SIMPLE_DATA:
			if (message->HasRef("refs"))
				ItemsReceived(message, fDropIndex);
			else if (message->HasPointer("list"))
				SimpleListView::MessageReceived(message);
			break;
		case B_REFS_RECEIVED:
			ItemsReceived(message, fDropIndex);
			break;

		default:
			SimpleListView::MessageReceived(message);
			break;
	}
}


void
PlaylistListView::MouseDown(BPoint where)
{
	if (!IsFocus())
		MakeFocus(true);

	int32 clicks;
	if (Window()->CurrentMessage()->FindInt32("clicks", &clicks) < B_OK)
		clicks = 1;

	bool handled = false;

	float playbackMarkSize = playback_mark_size(fFontHeight);
	float textOffset = text_offset(fFontHeight);

	for (int32 i = 0;
		Item* item = dynamic_cast<Item*>(ItemAt(i)); i++) {
		BRect r = ItemFrame(i);
		if (r.Contains(where)) {
			if (clicks == 2) {
				// only do something if user clicked the same item twice
				if (fLastClickedItem == item) {
					BAutolock _(fPlaylist);
					fPlaylist->SetCurrentItemIndex(i, true);
					handled = true;
				}
			} else {
				// remember last clicked item
				fLastClickedItem = item;
				if (i == fCurrentPlaylistIndex) {
					r.right = r.left + playbackMarkSize + textOffset;
					if (r.Contains (where)) {
						fController->TogglePlaying();
						handled = true;
					}
				}
			}
			break;
		}
	}

	if (!handled)
		SimpleListView::MouseDown(where);
}


void
PlaylistListView::KeyDown(const char* bytes, int32 numBytes)
{
	if (numBytes < 1)
		return;

	BMessage* msg = Window()->CurrentMessage();
	uint32 modifier = msg->FindInt32("modifiers");

	int32 count;
	int32 index;

	switch (bytes[0]) {
		case B_SPACE:
			fController->TogglePlaying();
			break;

		case B_BACKSPACE:
		case B_DELETE:
			RemoveSelected();
			break;

		case B_ENTER:
			count = CountItems();
			if (count == 0)
				break;
			index = CurrentSelection(0);
			if (index < 0)
				break;
			fPlaylist->SetCurrentItemIndex(index, true);
			fController->Play();
			break;

		case B_ESCAPE:
			fController->Stop();
			break;

		case B_RIGHT_ARROW:
			if ((modifier & B_SHIFT_KEY) != 0)
				_Wind(30000000LL, 5);
			else
				_Wind(5000000LL, 1);
			break;

		case B_LEFT_ARROW:
			if ((modifier & B_SHIFT_KEY) != 0)
				_Wind(-30000000LL, -5);
			else
				_Wind(-5000000LL, -1);
			break;
		default:
			DragSortableListView::KeyDown(bytes, numBytes);
	}
}


void
PlaylistListView::SkipBackward()
{
	BAutolock _(fPlaylist);
	int32 index = fPlaylist->CurrentItemIndex() - 1;
	if (index < 0)
		index = 0;
	fPlaylist->SetCurrentItemIndex(index, true);
}


void
PlaylistListView::SkipForward()
{
	BAutolock _(fPlaylist);
	int32 index = fPlaylist->CurrentItemIndex() + 1;
	if (index >= fPlaylist->CountItems())
		index = fPlaylist->CountItems() - 1;
	fPlaylist->SetCurrentItemIndex(index, true);
}


void
PlaylistListView::_Wind(bigtime_t howMuch, int64 frames)
{
	if (!fController->Lock())
		return;

	if (frames != 0 && !fController->IsPlaying()) {
		int64 newFrame = fController->CurrentFrame() + frames;
		fController->SetFramePosition(newFrame);
	} else {
		bigtime_t seekTime = fController->TimePosition() + howMuch;
		if (seekTime < 0) {
			SkipBackward();
		} else if (seekTime > fController->TimeDuration()) {
			SkipForward();
		} else
			fController->SetTimePosition(seekTime);
	}

	fController->Unlock();
}


void
PlaylistListView::MoveItems(const BList& indices, int32 toIndex)
{
	fCommandStack->Perform(new (nothrow) MovePLItemsCommand(fPlaylist,
		(int32*)indices.Items(), indices.CountItems(), toIndex));
}


void
PlaylistListView::CopyItems(const BList& indices, int32 toIndex)
{
	fCommandStack->Perform(new (nothrow) CopyPLItemsCommand(fPlaylist,
		(int32*)indices.Items(), indices.CountItems(), toIndex));
}


void
PlaylistListView::RemoveItemList(const BList& indices)
{
	RemoveItemList(indices, false);
}


void
PlaylistListView::DrawListItem(BView* owner, int32 index, BRect frame) const
{
	if (Item* item = dynamic_cast<Item*>(ItemAt(index))) {
		item->Draw(owner, frame, fFontHeight, index % 2,
			DISPLAY_NAME, index == fCurrentPlaylistIndex, fPlaybackState);
	}
}


void
PlaylistListView::ItemsReceived(const BMessage* message, int32 appendIndex)
{
	BPoint dropPoint;
	bool sorting = false;
	entry_ref ref;

	if (message->FindRef("refs", 1, &ref) == B_OK
		&& message->FindPoint("_drop_point_", &dropPoint) == B_OK
		&& message->GetInt32("buttons", 0) == 2)
		if (_ShowDropContextMenu(dropPoint) == M_ADD_SORTED)
			sorting = true;

	if (fCommandStack->Perform(new (nothrow) ImportPLItemsCommand(fPlaylist,
			message, appendIndex, sorting)) != B_OK) {
		fPlaylist->NotifyImportFailed();
	}
}


void
PlaylistListView::Randomize()
{
	int32 count = CountItems();
	if (count == 0)
		return;

	BList indices;

	// add current selection
	count = 0;
	while (true) {
		int32 index = CurrentSelection(count);
		if (index < 0)
			break;
		if (!indices.AddItem((void*)(addr_t)index))
			return;
		count++;
	}

	// was anything selected?
	if (count == 0) {
		// no selection, simply add all items
		count = CountItems();
		for (int32 i = 0; i < count; i++) {
			if (!indices.AddItem((void*)(addr_t)i))
				return;
		}
	}

	fCommandStack->Perform(new (nothrow) RandomizePLItemsCommand(fPlaylist,
		(int32*)indices.Items(), indices.CountItems()));
}


void
PlaylistListView::RemoveSelectionToTrash()
{
	BList indices;
	GetSelectedItems(indices);
	RemoveItemList(indices, true);
}


void
PlaylistListView::RemoveToTrash(int32 index)
{
	BList indices;
	indices.AddItem((void*)(addr_t)index);
	RemoveItemList(indices, true);
}


void
PlaylistListView::RemoveItemList(const BList& indices, bool intoTrash)
{
	fCommandStack->Perform(new (nothrow) RemovePLItemsCommand(fPlaylist,
		(int32*)indices.Items(), indices.CountItems(), intoTrash));
}


// #pragma mark -


void
PlaylistListView::_FullSync()
{
	if (!fPlaylist->Lock())
		return;

	// detaching the scrollbar temporarily will
	// make this much quicker
	BScrollBar* scrollBar = ScrollBar(B_VERTICAL);
	if (scrollBar) {
		if (Window())
			Window()->UpdateIfNeeded();
		scrollBar->SetTarget((BView*)NULL);
	}

	for (int32 i = CountItems() - 1; i >= 0; i--)
		_RemoveItem(i);

	int32 count = fPlaylist->CountItems();
	for (int32 i = 0; i < count; i++)
		_AddItem(fPlaylist->ItemAt(i), i);

	_SetCurrentPlaylistIndex(fPlaylist->CurrentItemIndex());
	_SetPlaybackState(fController->PlaybackState());

	// reattach scrollbar and sync it by calling FrameResized()
	if (scrollBar) {
		scrollBar->SetTarget(this);
		FrameResized(Bounds().Width(), Bounds().Height());
	}

	fPlaylist->Unlock();
}


void
PlaylistListView::_AddItem(PlaylistItem* _item, int32 index)
{
	if (_item == NULL)
		return;

	Item* item = new (nothrow) Item(_item);
	if (item != NULL)
		AddItem(item, index);
}


void
PlaylistListView::_RemoveItem(int32 index)
{
	delete RemoveItem(index);
}


void
PlaylistListView::_SetCurrentPlaylistIndex(int32 index)
{
	if (fCurrentPlaylistIndex == index)
		return;

	InvalidateItem(fCurrentPlaylistIndex);
	fCurrentPlaylistIndex = index;
	InvalidateItem(fCurrentPlaylistIndex);
}


void
PlaylistListView::_SetPlaybackState(uint32 state)
{
	if (fPlaybackState == state)
		return;

	fPlaybackState = state;
	InvalidateItem(fCurrentPlaylistIndex);
}


void
PlaylistListView::_AddDropContextMenu()
{
	fDropContextMenu = new BPopUpMenu("DropContext");

	fDropContextMenu->AddItem(new BMenuItem(B_TRANSLATE("Add sorted"),
		new BMessage(M_ADD_SORTED)));
	fDropContextMenu->AddItem(new BMenuItem(B_TRANSLATE("Add unsorted"),
		new BMessage(M_ADD_UNSORTED)));
}


uint32
PlaylistListView::_ShowDropContextMenu(BPoint dropPoint)
{
	BMenuItem* item;

	item = fDropContextMenu->Go(dropPoint, true, true);
	if (item != NULL)
		return item->Command();
	return 0;
}


