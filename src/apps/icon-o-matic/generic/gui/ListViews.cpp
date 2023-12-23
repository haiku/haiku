/*
 * Copyright 2006, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */

#include "ListViews.h"

#include <malloc.h>
#include <stdio.h>
#include <typeinfo>

#include <Bitmap.h>
#include <Clipboard.h>
#include <Cursor.h>
#include <Entry.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <StackOrHeapArray.h>
#include <String.h>
#include <Window.h>

#include "cursors.h"

#include "Selection.h"

#define MAX_DRAG_HEIGHT		200.0
#define ALPHA				170
#define TEXT_OFFSET			5.0


static const rgb_color kDropIndicatorColor = make_color(255, 65, 54, 255);
static const rgb_color kDragFrameColor = make_color(17, 17, 17, 255);


// #pragma mark - SimpleItem


SimpleItem::SimpleItem(const char *name)
	:
	BStringItem(name)
{
}


SimpleItem::~SimpleItem()
{
}


void
SimpleItem::DrawItem(BView* owner, BRect itemFrame, bool even)
{
	DrawBackground(owner, itemFrame, even);

	// label
	if (IsSelected())
		owner->SetHighUIColor(B_LIST_SELECTED_ITEM_TEXT_COLOR);
	else
		owner->SetHighUIColor(B_LIST_ITEM_TEXT_COLOR);

	font_height fh;
	owner->GetFontHeight(&fh);

	const char* text = Text();
	BString truncatedString(text);
	owner->TruncateString(&truncatedString, B_TRUNCATE_MIDDLE,
		itemFrame.Width() - TEXT_OFFSET - 4);

	float height = itemFrame.Height();
	float textHeight = fh.ascent + fh.descent;
	BPoint textPoint;
	textPoint.x = itemFrame.left + TEXT_OFFSET;
	textPoint.y = itemFrame.top
		+ ceilf(height / 2 - textHeight / 2 + fh.ascent);

	owner->DrawString(truncatedString.String(), textPoint);
}


void
SimpleItem::DrawBackground(BView* owner, BRect itemFrame, bool even)
{
	rgb_color bgColor;
	if (!IsEnabled()) {
		rgb_color textColor = ui_color(B_LIST_ITEM_TEXT_COLOR);
		rgb_color disabledColor;
		if (textColor.red + textColor.green + textColor.blue > 128 * 3)
			disabledColor = tint_color(textColor, B_DARKEN_2_TINT);
		else
			disabledColor = tint_color(textColor, B_LIGHTEN_2_TINT);
		bgColor = disabledColor;
	} else if (IsSelected())
		bgColor = ui_color(B_LIST_SELECTED_BACKGROUND_COLOR);
	else
		bgColor = ui_color(B_LIST_BACKGROUND_COLOR);

	if (even)
		bgColor = tint_color(bgColor, 1.06);

	owner->SetLowColor(bgColor);
	owner->FillRect(itemFrame, B_SOLID_LOW);
}


// #pragma mark - DragSortableListView


DragSortableListView::DragSortableListView(BRect frame, const char* name,
	list_view_type type, uint32 resizingMode, uint32 flags)
	:
	BListView(frame, name, type, resizingMode, flags),
	fDropRect(0, 0, -1, -1),
	fMouseWheelFilter(NULL),
	fScrollPulse(NULL),
	fDropIndex(-1),
	fLastClickedItem(NULL),
	fScrollView(NULL),
	fDragCommand(B_SIMPLE_DATA),
	fFocusedIndex(-1),
	fSelection(NULL),
	fSyncingToSelection(false),
	fModifyingSelection(false)
{
	SetViewColor(B_TRANSPARENT_32_BIT);
}


DragSortableListView::~DragSortableListView()
{
	delete fMouseWheelFilter;

	SetSelection(NULL);
}


void
DragSortableListView::AttachedToWindow()
{
	if (!fMouseWheelFilter)
		fMouseWheelFilter = new MouseWheelFilter(this);
	Window()->AddCommonFilter(fMouseWheelFilter);

	BListView::AttachedToWindow();

	// work arround a bug in BListView
	BRect bounds = Bounds();
	BListView::FrameResized(bounds.Width(), bounds.Height());
}


void
DragSortableListView::DetachedFromWindow()
{
	Window()->RemoveCommonFilter(fMouseWheelFilter);
}


void
DragSortableListView::FrameResized(float width, float height)
{
	BListView::FrameResized(width, height);
}


void
DragSortableListView::TargetedByScrollView(BScrollView* scrollView)
{
	fScrollView = scrollView;
	BListView::TargetedByScrollView(scrollView);
}


void
DragSortableListView::WindowActivated(bool active)
{
	// work-around for buggy focus indicator on BScrollView
	if (BView* view = Parent())
		view->Invalidate();
}


void
DragSortableListView::MessageReceived(BMessage* message)
{
	if (message->what == fDragCommand) {
		int32 count = CountItems();
		if (fDropIndex < 0 || fDropIndex > count)
			fDropIndex = count;
		HandleDropMessage(message, fDropIndex);
		fDropIndex = -1;
	} else {
		switch (message->what) {
			case B_MOUSE_WHEEL_CHANGED:
			{
				BListView::MessageReceived(message);
				BPoint where;
				uint32 buttons;
				GetMouse(&where, &buttons, false);
				uint32 transit = Bounds().Contains(where) ? B_INSIDE_VIEW : B_OUTSIDE_VIEW;
				MouseMoved(where, transit, &fDragMessageCopy);
				break;
			}

			default:
				BListView::MessageReceived(message);
				break;
		}
	}
}


void
DragSortableListView::KeyDown(const char* bytes, int32 numBytes)
{
	if (numBytes < 1)
		return;
		
	if ((bytes[0] == B_BACKSPACE) || (bytes[0] == B_DELETE))
		RemoveSelected();

	BListView::KeyDown(bytes, numBytes);
}


void
DragSortableListView::MouseDown(BPoint where)
{
	int32 index = IndexOf(where);
	BListItem* item = ItemAt(index);

	// bail out if item not found
	if (index < 0 || item == NULL) {
		fLastClickedItem = NULL;
		return BListView::MouseDown(where);
	}

	int32 clicks = 1;
	int32 buttons = 0;
	Window()->CurrentMessage()->FindInt32("clicks", &clicks);
	Window()->CurrentMessage()->FindInt32("buttons", &buttons);

	if (clicks == 2 && item == fLastClickedItem) {
		// only do something if user clicked the same item twice
		DoubleClicked(index);
	} else {
		// remember last clicked item
		fLastClickedItem = item;
	}

	if (ListType() == B_MULTIPLE_SELECTION_LIST
		&& (buttons & B_SECONDARY_MOUSE_BUTTON) != 0) {
		if (item->IsSelected())
			Deselect(index);
		else
			Select(index, true);
	} else
		BListView::MouseDown(where);
}


void
DragSortableListView::MouseMoved(BPoint where, uint32 transit, const BMessage* msg)
{
	int32 buttons = 0;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons);

	// only start a drag if a button is down and we have a drag message
	if (buttons > 0 && msg && AcceptDragMessage(msg)) {
		// we have dragged off the mouse down item
		// turn on auto-scrolling and drag and drop
		switch (transit) {
			case B_ENTERED_VIEW:
			case B_INSIDE_VIEW:
				// remember drag message to react on modifier changes
				SetDragMessage(msg);
				// set drop target through virtual function
				SetDropTargetRect(msg, where);
			break;

			case B_EXITED_VIEW:
				// forget drag message
				SetDragMessage(NULL);
				// don't draw drop rect indicator
				InvalidateDropRect();
			case B_OUTSIDE_VIEW:
				break;
		}
	} else {
		// be sure to forget drag message
		SetDragMessage(NULL);
		// don't draw drop rect indicator
		InvalidateDropRect();
		// restore hand cursor
		BCursor cursor(B_HAND_CURSOR);
		SetViewCursor(&cursor, true);
	}

	fLastMousePos = where;
	BListView::MouseMoved(where, transit, msg);
}


void
DragSortableListView::MouseUp(BPoint where)
{
	// turn off auto-scrolling
	BListView::MouseUp(where);
	// be sure to forget drag message
	SetDragMessage(NULL);
	// don't draw drop rect indicator
	InvalidateDropRect();
	// restore hand cursor
	BCursor cursor(B_HAND_CURSOR);
	SetViewCursor(&cursor, true);
}


bool
DragSortableListView::MouseWheelChanged(float x, float y)
{
	BPoint where;
	uint32 buttons;
	GetMouse(&where, &buttons, false);
	if (Bounds().Contains(where))
		return true;
	else
		return false;
}


// #pragma mark -


void
DragSortableListView::ObjectChanged(const Observable* object)
{
	if (object != fSelection || fModifyingSelection || fSyncingToSelection)
		return;

//printf("%s - syncing start\n", Name());
	fSyncingToSelection = true;

	// try to sync to Selection
	BList selectedItems;

	int32 count = fSelection->CountSelected();
	for (int32 i = 0; i < count; i++) {
		int32 index = IndexOfSelectable(fSelection->SelectableAtFast(i));
		if (index >= 0) {
			BListItem* item = ItemAt(index);
			if (item && !selectedItems.HasItem((void*)item))
				selectedItems.AddItem((void*)item);
		}
	}

	count = selectedItems.CountItems();
	if (count == 0) {
		if (CurrentSelection(0) >= 0)
			DeselectAll();
	} else {
		count = CountItems();
		for (int32 i = 0; i < count; i++) {
			BListItem* item = ItemAt(i);
			bool selected = selectedItems.RemoveItem((void*)item);
			if (item->IsSelected() != selected) {
				Select(i, true);
			}
		}
	}

	fSyncingToSelection = false;
//printf("%s - done\n", Name());
}


// #pragma mark -


void
DragSortableListView::SetDragCommand(uint32 command)
{
	fDragCommand = command;
}


void
DragSortableListView::ModifiersChanged()
{
	SetDropTargetRect(&fDragMessageCopy, fLastMousePos);
}


void
DragSortableListView::SetItemFocused(int32 index)
{
	InvalidateItem(fFocusedIndex);
	InvalidateItem(index);
	fFocusedIndex = index;
}


bool
DragSortableListView::AcceptDragMessage(const BMessage* message) const
{
	return message->what == fDragCommand;
}


void
DragSortableListView::SetDropTargetRect(const BMessage* message, BPoint where)
{
	if (AcceptDragMessage(message)) {
		bool copy = modifiers() & B_SHIFT_KEY;
		bool replaceAll = !message->HasPointer("list") && !copy;
		BRect rect = Bounds();
		if (replaceAll) {
			// compensate for scrollbar offset
			rect.bottom--;
			fDropRect = rect;
			fDropIndex = -1;
		} else {
			// offset where by half of item height
			rect = ItemFrame(0);
			where.y += rect.Height() / 2;

			int32 index = IndexOf(where);
			if (index < 0)
				index = CountItems();
			SetDropIndex(index);

			const uchar* cursorData = copy ? kCopyCursor : B_HAND_CURSOR;
			BCursor cursor(cursorData);
			SetViewCursor(&cursor, true);
		}
	}
}


bool
DragSortableListView::HandleDropMessage(const BMessage* message,
	int32 dropIndex)
{
	DragSortableListView *list = NULL;
	if (message->FindPointer("list", (void **)&list) != B_OK || list != this)
		return false;

	BList items;
	int32 index;
	for (int32 i = 0; message->FindInt32("index", i, &index) == B_OK; i++) {
		BListItem* item = ItemAt(index);
		if (item != NULL)
			items.AddItem((void*)item);
	}

	if (items.CountItems() == 0)
		return false;

	if ((modifiers() & B_SHIFT_KEY) != 0)
		CopyItems(items, dropIndex);
	else
		MoveItems(items, dropIndex);

	return true;
}


bool
DragSortableListView::DoesAutoScrolling() const
{
	return true;
}


void
DragSortableListView::MoveItems(BList& items, int32 index)
{
	DeselectAll();
	// we remove the items while we look at them, the insertion index is decreased
	// when the items index is lower, so that we insert at the right spot after
	// removal
	BList removedItems;
	int32 count = items.CountItems();
	for (int32 i = 0; i < count; i++) {
		BListItem* item = (BListItem*)items.ItemAt(i);
		int32 removeIndex = IndexOf(item);
		if (RemoveItem(item) && removedItems.AddItem((void*)item)) {
			if (removeIndex < index)
				index--;
		}
		// else ??? -> blow up
	}
	for (int32 i = 0;
		 BListItem* item = (BListItem*)removedItems.ItemAt(i); i++) {
		if (AddItem(item, index)) {
			// after we're done, the newly inserted items will be selected
			Select(index, true);
			// next items will be inserted after this one
			index++;
		} else
			delete item;
	}
}


void
DragSortableListView::CopyItems(BList& items, int32 index)
{
	DeselectAll();
	// by inserting the items after we copied all items first, we avoid
	// cloning an item we already inserted and messing everything up
	// in other words, don't touch the list before we know which items
	// need to be cloned
	BList clonedItems;
	int32 count = items.CountItems();
	for (int32 i = 0; i < count; i++) {
		BListItem* item = CloneItem(IndexOf((BListItem*)items.ItemAt(i)));
		if (item && !clonedItems.AddItem((void*)item))
			delete item;
	}
	for (int32 i = 0;
		 BListItem* item = (BListItem*)clonedItems.ItemAt(i); i++) {
		if (AddItem(item, index)) {
			// after we're done, the newly inserted items will be selected
			Select(index, true);
			// next items will be inserted after this one
			index++;
		} else
			delete item;
	}
}


void
DragSortableListView::RemoveItemList(BList& items)
{
	int32 count = items.CountItems();
	for (int32 i = 0; i < count; i++) {
		BListItem* item = (BListItem*)items.ItemAt(i);
		if (RemoveItem(item))
			delete item;
	}
}


void
DragSortableListView::RemoveSelected()
{
//	if (fFocusedIndex >= 0)
//		return;

	BList items;
	for (int32 i = 0; BListItem* item = ItemAt(CurrentSelection(i)); i++)
		items.AddItem((void*)item);
	RemoveItemList(items);
}


// #pragma mark -


void
DragSortableListView::SetSelection(Selection* selection)
{
	if (fSelection == selection)
		return;

	if (fSelection)
		fSelection->RemoveObserver(this);

	fSelection = selection;

	if (fSelection)
		fSelection->AddObserver(this);
}


int32
DragSortableListView::IndexOfSelectable(Selectable* selectable) const
{
	return -1;
}


Selectable*
DragSortableListView::SelectableFor(BListItem* item) const
{
	return NULL;
}


void
DragSortableListView::SelectAll()
{
	Select(0, CountItems() - 1);
}


int32
DragSortableListView::CountSelectedItems() const
{
	int32 count = 0;
	while (CurrentSelection(count) >= 0)
		count++;
	return count;
}


void
DragSortableListView::SelectionChanged()
{
//printf("%s::SelectionChanged()", typeid(*this).name());
	// modify global Selection
	if (!fSelection || fSyncingToSelection)
		return;

	fModifyingSelection = true;

	BList selectables;
	for (int32 i = 0; BListItem* item = ItemAt(CurrentSelection(i)); i++) {
		Selectable* selectable = SelectableFor(item);
		if (selectable)
			selectables.AddItem((void*)selectable);
	}

	AutoNotificationSuspender _(fSelection);

	int32 count = selectables.CountItems();
	if (count == 0) {
//printf("  deselecting all\n");
		if (!fSyncingToSelection)
			fSelection->DeselectAll();
	} else {
//printf("  selecting %ld items\n", count);
		for (int32 i = 0; i < count; i++) {
			Selectable* selectable = (Selectable*)selectables.ItemAtFast(i);
			fSelection->Select(selectable, i > 0);
		}
	}

	fModifyingSelection = false;
}


// #pragma mark -


bool
DragSortableListView::DeleteItem(int32 index)
{
	BListItem* item = ItemAt(index);
	if (item && RemoveItem(item)) {
		delete item;
		return true;
	}
	return false;
}


void
DragSortableListView::SetDropRect(BRect rect)
{
	fDropRect = rect;
}


void
DragSortableListView::SetDropIndex(int32 index)
{
	if (fDropIndex != index) {
		fDropIndex = index;
		if (fDropIndex >= 0) {
			int32 count = CountItems();
			if (fDropIndex == count) {
				BRect rect;
				if (ItemAt(count - 1)) {
					rect = ItemFrame(count - 1);
					rect.top = rect.bottom;
					rect.bottom = rect.top + 1;
				} else {
					rect = Bounds();
					// compensate for scrollbars moved slightly out of window
					rect.bottom--;
				}
				fDropRect = rect;
			} else {
				BRect rect = ItemFrame(fDropIndex);
				rect.top--;
				rect.bottom = rect.top + 1;
				fDropRect = rect;
			}
		}
	}
}


void
DragSortableListView::InvalidateDropRect()
{
	fDropRect = BRect(0, 0, -1, -1);
//	SetDropIndex(-1);
}


void
DragSortableListView::SetDragMessage(const BMessage* message)
{
	if (message)
		fDragMessageCopy = *message;
	else
		fDragMessageCopy.what = 0;
}


// #pragma mark - SimpleListView


SimpleListView::SimpleListView(BRect frame, BMessage* selectionChangeMessage)
	:
	DragSortableListView(frame, "playlist listview",
		B_MULTIPLE_SELECTION_LIST, B_FOLLOW_ALL,
		B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE),
		fSelectionChangeMessage(selectionChangeMessage)
{
}


SimpleListView::SimpleListView(BRect frame, const char* name,
	BMessage* selectionChangeMessage, list_view_type type,
	uint32 resizingMode, uint32 flags)
	:
	DragSortableListView(frame, name, type, resizingMode, flags),
		fSelectionChangeMessage(selectionChangeMessage)
{
}


SimpleListView::~SimpleListView()
{
	delete fSelectionChangeMessage;
}


void
SimpleListView::DetachedFromWindow()
{
	DragSortableListView::DetachedFromWindow();
	_MakeEmpty();
}


void
SimpleListView::Draw(BRect updateRect)
{
	BRect emptyRect = updateRect;

	int32 firstIndex = IndexOf(updateRect.LeftTop());
	int32 lastIndex = IndexOf(updateRect.RightBottom());
	if (firstIndex >= 0) {
		BListItem* item;
		BRect itemFrame(0, 0, -1, -1);
		if (lastIndex < firstIndex)
			lastIndex = CountItems() - 1;
		// update rect contains items
		for (int32 i = firstIndex; i <= lastIndex; i++) {
			item = ItemAt(i);
			if (item == NULL)
				continue;
			itemFrame = ItemFrame(i);
			item->DrawItem(this, itemFrame, (i % 2) == 0);

			// drop indicator
			if (i == fDropIndex) {
				SetHighColor(kDropIndicatorColor);
				StrokeLine(fDropRect.LeftTop(), fDropRect.RightTop());
			}
		}
		emptyRect.top = itemFrame.bottom + 1;
	}

	if (emptyRect.IsValid()) {
		SetLowUIColor(B_LIST_BACKGROUND_COLOR);
		FillRect(emptyRect, B_SOLID_LOW);
	}

#if 0
	// focus indicator
	if (IsFocus()) {
		SetHighUIColor(B_KEYBOARD_NAVIGATION_COLOR);
		StrokeRect(Bounds());
	}
#endif
}


bool
SimpleListView::InitiateDrag(BPoint where, int32 index, bool)
{
	// supress drag & drop while an item is focused
	if (fFocusedIndex >= 0)
		return false;

	BListItem* item = ItemAt(CurrentSelection(0));
	if (item == NULL) {
		// work-around a timing problem
		Select(index);
		item = ItemAt(index);
	}
	if (item == NULL)
		return false;

	// create drag message
	BMessage msg(fDragCommand);
	MakeDragMessage(&msg);
	// figure out drag rect
	float width = Bounds().Width();
	BRect dragRect(0, 0, width, -1);
	// figure out how many items fit into our bitmap
	int32 numItems;
	bool fade = false;
	for (numItems = 0; BListItem* item = ItemAt(CurrentSelection(numItems)); numItems++) {
		dragRect.bottom += ceilf(item->Height()) + 1;
		if (dragRect.Height() > MAX_DRAG_HEIGHT) {
			fade = true;
			dragRect.bottom = MAX_DRAG_HEIGHT;
			numItems++;
			break;
		}
	}

	BBitmap* dragBitmap = new BBitmap(dragRect, B_RGBA32, true);
	if (dragBitmap && dragBitmap->IsValid()) {
		if (BView* view = new BView(dragBitmap->Bounds(), "helper",
				B_FOLLOW_NONE, B_WILL_DRAW)) {
			dragBitmap->AddChild(view);
			dragBitmap->Lock();
			BRect itemFrame(dragRect) ;
			itemFrame.bottom = 0.0;
			BListItem* item;
			// let all selected items, that fit into our drag_bitmap, draw
			for (int32 i = 0; i < numItems; i++) {
				item = ItemAt(CurrentSelection(i));
				if (item == NULL)
					continue;
				itemFrame.bottom = itemFrame.top + ceilf(item->Height());
				if (itemFrame.bottom > dragRect.bottom)
					itemFrame.bottom = dragRect.bottom;
				item->DrawItem(view, itemFrame, (i % 2) == 0);
				itemFrame.top = itemFrame.bottom + 1;
			}

			// stroke a black frame around the edge
			view->SetHighColor(kDragFrameColor);
			view->StrokeRect(view->Bounds());
			view->Sync();

			uint8* bits = (uint8*)dragBitmap->Bits();
			int32 height = (int32)dragBitmap->Bounds().Height() + 1;
			int32 width = (int32)dragBitmap->Bounds().Width() + 1;
			int32 bpr = dragBitmap->BytesPerRow();

			if (fade) {
				for (int32 y = 0; y < height - ALPHA / 2; y++, bits += bpr) {
					uint8* line = bits + 3;
					for (uint8 *end = line + 4 * width; line < end; line += 4)
						*line = ALPHA;
				}
				for (int32 y = height - ALPHA / 2; y < height; y++, bits += bpr) {
					uint8* line = bits + 3;
					for (uint8 *end = line + 4 * width; line < end; line += 4)
						*line = (height - y) << 1;
				}
			} else {
				for (int32 y = 0; y < height; y++, bits += bpr) {
					uint8* line = bits + 3;
					for (uint8 *end = line + 4 * width; line < end; line += 4)
						*line = ALPHA;
				}
			}
			dragBitmap->Unlock();
		}
	} else {
		delete dragBitmap;
		dragBitmap = NULL;
	}

	if (dragBitmap)
		DragMessage(&msg, dragBitmap, B_OP_ALPHA, B_ORIGIN);
	else
		DragMessage(&msg, dragRect.OffsetToCopy(where), this);

	SetDragMessage(&msg);

	return true;
}


void
SimpleListView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		// NOTE: pasting is handled in MainWindow::MessageReceived
		case B_COPY:
		{
			int count = CountSelectedItems();
			if (count == 0)
				return;

			if (!be_clipboard->Lock())
				break;
			be_clipboard->Clear();

			BMessage data;
			ArchiveSelection(&data);

			ssize_t size = data.FlattenedSize();
			BStackOrHeapArray<char, 1024> archive(size);
			if (!archive) {
				be_clipboard->Unlock();
				break;
			}
			data.Flatten(archive, size);

			be_clipboard->Data()->AddData(
				"application/x-vnd.icon_o_matic-listview-message", B_MIME_TYPE, archive, size);

			be_clipboard->Commit();
			be_clipboard->Unlock();

			break;
		}

		default:
			DragSortableListView::MessageReceived(message);
			break;
	}
}


BListItem*
SimpleListView::CloneItem(int32 atIndex) const
{
	BListItem* clone = NULL;
	if (SimpleItem* item = dynamic_cast<SimpleItem*>(ItemAt(atIndex)))
		clone = new SimpleItem(item->Text());
	return clone;
}


void
SimpleListView::MakeDragMessage(BMessage* message) const
{
	if (message) {
		message->AddPointer("list",
			(void*)dynamic_cast<const DragSortableListView*>(this));
		int32 index;
		for (int32 i = 0; (index = CurrentSelection(i)) >= 0; i++)
			message->AddInt32("index", index);
	}

	BMessage items;
	ArchiveSelection(&items);
	message->AddMessage("items", &items);
}


bool
SimpleListView::HandleDropMessage(const BMessage* message, int32 dropIndex)
{
	// Let DragSortableListView handle drag-sorting (when drag came from ourself)
	if (DragSortableListView::HandleDropMessage(message, dropIndex))
		return true;

	BMessage items;
	if (message->FindMessage("items", &items) != B_OK)
		return false;

	return InstantiateSelection(&items, dropIndex);
}


bool
SimpleListView::HandlePaste(const BMessage* archive)
{
	return InstantiateSelection(archive, CountItems());
}


void
SimpleListView::_MakeEmpty()
{
	// NOTE: BListView::MakeEmpty() uses ScrollTo()
	// for which the object needs to be attached to
	// a BWindow.... :-(
	int32 count = CountItems();
	for (int32 i = count - 1; i >= 0; i--)
		delete RemoveItem(i);
}

