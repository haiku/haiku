/*
 * Copyright 2006-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#include <stdio.h>
#include <malloc.h>
#include <new>

#include <Bitmap.h>
#include <Cursor.h>
#include <Entry.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <String.h>
#include <Window.h>

#include "ListViews.h"

const unsigned char kCopyCursor[] = { 16, 1, 1, 1,
	0x00, 0x00, 0x70, 0x00, 0x48, 0x00, 0x48, 0x00,
	0x27, 0xc0, 0x24, 0xb8, 0x12, 0x54, 0x10, 0x02,
	0x79, 0xe2, 0x99, 0x22, 0x85, 0x7a, 0x61, 0x4a,
	0x19, 0xca, 0x04, 0x4a, 0x02, 0x78, 0x00, 0x00,

	0x00, 0x00, 0x70, 0x00, 0x78, 0x00, 0x78, 0x00,
	0x3f, 0xc0, 0x3f, 0xf8, 0x1f, 0xfc, 0x1f, 0xfe,
	0x7f, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0x7f, 0xfe,
	0x1f, 0xfe, 0x07, 0xfe, 0x03, 0xf8, 0x00, 0x00 };

#define MAX_DRAG_HEIGHT		200.0
#define ALPHA				170
#define TEXT_OFFSET			5.0

enum {
	MSG_TICK	= 'tick',
};

using std::nothrow;

// SimpleItem class
SimpleItem::SimpleItem( const char *name )
	: BStringItem( name )
{
}

SimpleItem::~SimpleItem()
{
}

// SimpleItem::DrawItem
void
SimpleItem::Draw(BView *owner, BRect frame, uint32 flags)
{
	DrawBackground(owner, frame, flags);
	// label
	owner->SetHighColor( 0, 0, 0, 255 );
	font_height fh;
	owner->GetFontHeight( &fh );
	const char* text = Text();
	BString truncatedString( text );
	owner->TruncateString( &truncatedString, B_TRUNCATE_MIDDLE,
						   frame.Width() - TEXT_OFFSET - 4.0 );
	float height = frame.Height();
	float textHeight = fh.ascent + fh.descent;
	BPoint textPoint;
	textPoint.x = frame.left + TEXT_OFFSET;
	textPoint.y = frame.top
				  + ceilf(height / 2.0 - textHeight / 2.0
				  		  + fh.ascent);
	owner->DrawString(truncatedString.String(), textPoint);
}

// SimpleItem::DrawBackground
void
SimpleItem::DrawBackground(BView *owner, BRect frame, uint32 flags)
{
	// stroke a blue frame around the item if it's focused
	if (flags & FLAGS_FOCUSED) {
		owner->SetLowColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		owner->StrokeRect(frame, B_SOLID_LOW);
		frame.InsetBy(1.0, 1.0);
	}
	// figure out bg-color
	rgb_color color = (rgb_color){ 255, 255, 255, 255 };
	if ( flags & FLAGS_TINTED_LINE )
		color = tint_color( color, 1.06 );
	// background
	if ( IsSelected() )
		color = tint_color( color, B_DARKEN_2_TINT );
	owner->SetLowColor( color );
	owner->FillRect( frame, B_SOLID_LOW );
}

// DragSortableListView class
DragSortableListView::DragSortableListView(BRect frame, const char* name,
										   list_view_type type, uint32 resizingMode,
										   uint32 flags)
	: BListView(frame, name, type, resizingMode, flags),
	  fDropRect(0.0, 0.0, -1.0, -1.0),
	  fScrollPulse(NULL),
	  fDropIndex(-1),
	  fLastClickedItem(NULL),
	  fScrollView(NULL),
	  fDragCommand(B_SIMPLE_DATA),
	  fFocusedIndex(-1)
{
	SetViewColor(B_TRANSPARENT_32_BIT);
}

DragSortableListView::~DragSortableListView()
{
	delete fScrollPulse;
}

// AttachedToWindow
void
DragSortableListView::AttachedToWindow()
{
	BListView::AttachedToWindow();

	// work arround a bug in BListView
	BRect bounds = Bounds();
	BListView::FrameResized(bounds.Width(), bounds.Height());
}

// DetachedFromWindow
void
DragSortableListView::DetachedFromWindow()
{
}

// FrameResized
void
DragSortableListView::FrameResized(float width, float height)
{
	BListView::FrameResized(width, height);
	Invalidate();
}

/*
// MakeFocus
void
DragSortableListView::MakeFocus(bool focused)
{
	if (focused != IsFocus()) {
		Invalidate();
		BListView::MakeFocus(focused);
	}
}
*/
// Draw
void
DragSortableListView::Draw( BRect updateRect )
{
	int32 firstIndex = IndexOf(updateRect.LeftTop());
	int32 lastIndex = IndexOf(updateRect.RightBottom());
	if (firstIndex >= 0) {
		if (lastIndex < firstIndex)
			lastIndex = CountItems() - 1;
		// update rect contains items
		BRect r = updateRect;
		for (int32 i = firstIndex; i <= lastIndex; i++) {
			r = ItemFrame(i);
			DrawListItem(this, i, r);
		}
		updateRect.top = r.bottom + 1.0;
		if (updateRect.IsValid()) {
			SetLowColor(255, 255, 255, 255);
			FillRect(updateRect, B_SOLID_LOW);
		}
	} else {
		SetLowColor(255, 255, 255, 255);
		FillRect(updateRect, B_SOLID_LOW);
	}
	// drop anticipation indication
	if (fDropRect.IsValid()) {
		SetHighColor(255, 0, 0, 255);
		StrokeRect(fDropRect);
	}
/*	// focus indication
	if (IsFocus()) {
		SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		StrokeRect(Bounds());
	}*/
}

// ScrollTo
void
DragSortableListView::ScrollTo(BPoint where)
{
	uint32 buttons;
	BPoint point;
	GetMouse(&point, &buttons, false);
	uint32 transit = Bounds().Contains(point) ? B_INSIDE_VIEW : B_OUTSIDE_VIEW;
	MouseMoved(point, transit, &fDragMessageCopy);
	BListView::ScrollTo(where);
}

// TargetedByScrollView
void
DragSortableListView::TargetedByScrollView(BScrollView* scrollView)
{
	fScrollView = scrollView;
	BListView::TargetedByScrollView(scrollView);
}

// InitiateDrag
bool
DragSortableListView::InitiateDrag( BPoint point, int32 index, bool )
{
	// supress drag&drop while an item is focused
	if (fFocusedIndex >= 0)
		return false;

	bool success = false;
	BListItem* item = ItemAt( CurrentSelection( 0 ) );
	if ( !item ) {
		// workarround a timing problem
		Select( index );
		item = ItemAt( index );
	}
	if ( item ) {
		// create drag message
		BMessage msg( fDragCommand );
		MakeDragMessage( &msg );
		// figure out drag rect
		float width = Bounds().Width();
		BRect dragRect(0.0, 0.0, width, -1.0);
		// figure out, how many items fit into our bitmap
		int32 numItems;
		bool fade = false;
		for (numItems = 0; BListItem* item = ItemAt( CurrentSelection( numItems ) ); numItems++) {
			dragRect.bottom += ceilf( item->Height() ) + 1.0;
			if ( dragRect.Height() > MAX_DRAG_HEIGHT ) {
				fade = true;
				dragRect.bottom = MAX_DRAG_HEIGHT;
				numItems++;
				break;
			}
		}
		BBitmap* dragBitmap = new BBitmap( dragRect, B_RGB32, true );
		if ( dragBitmap && dragBitmap->IsValid() ) {
			if ( BView *v = new BView( dragBitmap->Bounds(), "helper", B_FOLLOW_NONE, B_WILL_DRAW ) ) {
				dragBitmap->AddChild( v );
				dragBitmap->Lock();
				BRect itemBounds( dragRect) ;
				itemBounds.bottom = 0.0;
				// let all selected items, that fit into our drag_bitmap, draw
				for ( int32 i = 0; i < numItems; i++ ) {
					int32 index = CurrentSelection( i );
					BListItem* item = ItemAt( index );
					itemBounds.bottom = itemBounds.top + ceilf( item->Height() );
					if ( itemBounds.bottom > dragRect.bottom )
						itemBounds.bottom = dragRect.bottom;
					DrawListItem( v, index, itemBounds );
					itemBounds.top = itemBounds.bottom + 1.0;
				}
				// make a black frame arround the edge
				v->SetHighColor( 0, 0, 0, 255 );
				v->StrokeRect( v->Bounds() );
				v->Sync();

				uint8 *bits = (uint8 *)dragBitmap->Bits();
				int32 height = (int32)dragBitmap->Bounds().Height() + 1;
				int32 width = (int32)dragBitmap->Bounds().Width() + 1;
				int32 bpr = dragBitmap->BytesPerRow();

				if (fade) {
					for ( int32 y = 0; y < height - ALPHA / 2; y++, bits += bpr ) {
						uint8 *line = bits + 3;
						for (uint8 *end = line + 4 * width; line < end; line += 4)
							*line = ALPHA;
					}
					for ( int32 y = height - ALPHA / 2; y < height; y++, bits += bpr ) {
						uint8 *line = bits + 3;
						for (uint8 *end = line + 4 * width; line < end; line += 4)
							*line = (height - y) << 1;
					}
				} else {
					for ( int32 y = 0; y < height; y++, bits += bpr ) {
						uint8 *line = bits + 3;
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
			DragMessage( &msg, dragBitmap, B_OP_ALPHA, BPoint( 0.0, 0.0 ) );
		else
			DragMessage( &msg, dragRect.OffsetToCopy( point ), this );

		_SetDragMessage(&msg);
		success = true;
	}
	return success;
}

// WindowActivated
void
DragSortableListView::WindowActivated( bool active )
{
	// workarround for buggy focus indication of BScrollView
	if ( BView* view = Parent() )
		view->Invalidate();
}

// MessageReceived
void
DragSortableListView::MessageReceived(BMessage* message)
{
	if (AcceptDragMessage(message)) {
		DragSortableListView *list = NULL;
		if (message->FindPointer("list", (void **)&list) == B_OK
			&& list == this) {
			int32 count = CountItems();
			if (fDropIndex < 0 || fDropIndex > count)
				fDropIndex = count;
			BList indices;
			int32 index;
			for (int32 i = 0; message->FindInt32("index", i, &index) == B_OK; i++)
				indices.AddItem((void*)index);
			if (indices.CountItems() > 0) {
				if (modifiers() & B_SHIFT_KEY)
					CopyItems(indices, fDropIndex);
				else
					MoveItems(indices, fDropIndex);
			}
			fDropIndex = -1;
		}
	} else {
		switch (message->what) {
			case MSG_TICK: {
				float scrollV = 0.0;
				BRect rect(Bounds());
				BPoint point;
				uint32 buttons;
				GetMouse(&point, &buttons, false);
				if (rect.Contains(point)) {
					// calculate the vertical scrolling offset
					float hotDist = rect.Height() * SCROLL_AREA;
					if (point.y > rect.bottom - hotDist)
						scrollV = hotDist - (rect.bottom - point.y);
					else if (point.y < rect.top + hotDist)
						scrollV = (point.y - rect.top) - hotDist;
				}
				// scroll
				if (scrollV != 0.0 && fScrollView) {
					if (BScrollBar* scrollBar = fScrollView->ScrollBar(B_VERTICAL)) {
						float value = scrollBar->Value();
						scrollBar->SetValue(scrollBar->Value() + scrollV);
						if (scrollBar->Value() != value) {
							// update mouse position
							uint32 buttons;
							BPoint point;
							GetMouse(&point, &buttons, false);
							uint32 transit = Bounds().Contains(point) ? B_INSIDE_VIEW : B_OUTSIDE_VIEW;
							MouseMoved(point, transit, &fDragMessageCopy);
						}
					}
				}
				break;
			}
			case B_MODIFIERS_CHANGED:
				ModifiersChanged();
				break;
			case B_MOUSE_WHEEL_CHANGED: {
				BListView::MessageReceived( message );
				BPoint point;
				uint32 buttons;
				GetMouse(&point, &buttons, false);
				uint32 transit = Bounds().Contains(point) ? B_INSIDE_VIEW : B_OUTSIDE_VIEW;
				MouseMoved(point, transit, &fDragMessageCopy);
				break;
			}
			default:
				BListView::MessageReceived( message );
				break;
		}
	}
}

// KeyDown
void
DragSortableListView::KeyDown( const char* bytes, int32 numBytes )
{
	if ( numBytes < 1 )
		return;

	if ( ( bytes[0] == B_BACKSPACE ) || ( bytes[0] == B_DELETE ) )
		RemoveSelected();

	BListView::KeyDown( bytes, numBytes );
}

// MouseDown
void
DragSortableListView::MouseDown( BPoint where )
{
	int32 clicks = 1;
	uint32 buttons = 0;
	Window()->CurrentMessage()->FindInt32("clicks", &clicks);
	Window()->CurrentMessage()->FindInt32("buttons", (int32*)&buttons);
	int32 clickedIndex = -1;
	for (int32 i = 0; BListItem* item = ItemAt(i); i++) {
		if (ItemFrame(i).Contains(where)) {
			if (clicks == 2) {
				// only do something if user clicked the same item twice
				if (fLastClickedItem == item)
					DoubleClicked(i);
			} else {
				// remember last clicked item
				fLastClickedItem = item;
			}
			clickedIndex = i;
			break;
		}
	}
	if (clickedIndex == -1)
		fLastClickedItem = NULL;

	BListItem* item = ItemAt(clickedIndex);
	if (ListType() == B_MULTIPLE_SELECTION_LIST
		&& item && (buttons & B_SECONDARY_MOUSE_BUTTON)) {
		if (item->IsSelected())
			Deselect(clickedIndex);
		else
			Select(clickedIndex, true);
	} else {
		BListView::MouseDown(where);
	}
}

// MouseMoved
void
DragSortableListView::MouseMoved(BPoint where, uint32 transit, const BMessage *msg)
{
	if (msg && AcceptDragMessage(msg)) {
		switch (transit) {
			case B_ENTERED_VIEW:
			case B_INSIDE_VIEW: {
				// remember drag message
				// this is needed to react on modifier changes
				_SetDragMessage(msg);
				// set drop target through virtual function
				SetDropTargetRect(msg, where);
				// go into autoscrolling mode
				BRect r = Bounds();
				r.InsetBy(0.0, r.Height() * SCROLL_AREA);
				SetAutoScrolling(!r.Contains(where));
				break;
			}
			case B_EXITED_VIEW:
				// forget drag message
				_SetDragMessage(NULL);
				SetAutoScrolling(false);
				// fall through
			case B_OUTSIDE_VIEW:
				_RemoveDropAnticipationRect();
				break;
		}
	} else {
		_RemoveDropAnticipationRect();
		BListView::MouseMoved(where, transit, msg);
		_SetDragMessage(NULL);
		SetAutoScrolling(false);

		BCursor cursor(B_HAND_CURSOR);
		SetViewCursor(&cursor, true);
	}
	fLastMousePos = where;
}

// MouseUp
void
DragSortableListView::MouseUp( BPoint where )
{
	// remove drop mark
	_SetDropAnticipationRect( BRect( 0.0, 0.0, -1.0, -1.0 ) );
	SetAutoScrolling(false);
	// be sure to forget drag message
	_SetDragMessage(NULL);
	BListView::MouseUp( where );

	BCursor cursor(B_HAND_CURSOR);
	SetViewCursor(&cursor, true);
}

// DrawItem
void
DragSortableListView::DrawItem( BListItem *item, BRect itemFrame, bool complete )
{
	DrawListItem( this, IndexOf( item ), itemFrame );
/*	if (IsFocus()) {
		SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		StrokeRect(Bounds());
	}*/
}

// MouseWheelChanged
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

// SetDragCommand
void
DragSortableListView::SetDragCommand(uint32 command)
{
	fDragCommand = command;
}

// ModifiersChaned
void
DragSortableListView::ModifiersChanged()
{
	SetDropTargetRect(&fDragMessageCopy, fLastMousePos);
}

// SetItemFocused
void
DragSortableListView::SetItemFocused(int32 index)
{
	InvalidateItem(fFocusedIndex);
	InvalidateItem(index);
	fFocusedIndex = index;
}

// AcceptDragMessage
bool
DragSortableListView::AcceptDragMessage(const BMessage* message) const
{
	return message->what == fDragCommand;
}

// SetDropTargetRect
void
DragSortableListView::SetDropTargetRect(const BMessage* message, BPoint where)

{
	if (AcceptDragMessage(message)) {
		bool copy = modifiers() & B_SHIFT_KEY;
		bool replaceAll = !message->HasPointer("list") && copy;
			// when dragging something in from ie Tracker, the
			// user has to hold shift down to replace everything
			// (opposite meaning of the normal shift behaviour)
		BRect r = Bounds();
		if (replaceAll) {
			r.bottom--;	// compensate for scrollbar offset
			_SetDropAnticipationRect(r);
			fDropIndex = -1;
		} else {
			// offset where by half of item height
			r = ItemFrame(0);
			where.y += r.Height() / 2.0;

			int32 index = IndexOf(where);
			if (index < 0)
				index = CountItems();
			_SetDropIndex(index);

			const uchar* cursorData = copy ? kCopyCursor : B_HAND_CURSOR;
			BCursor cursor(cursorData);
			SetViewCursor(&cursor, true);
		}
	}
}

// SetAutoScrolling
void
DragSortableListView::SetAutoScrolling(bool enable)
{
	if (fScrollPulse && enable)
		return;
	if (enable) {
		BMessenger messenger(this, Window());
		BMessage message(MSG_TICK);
		fScrollPulse = new BMessageRunner(messenger, &message, 40000LL);
	} else {
		delete fScrollPulse;
		fScrollPulse = NULL;
	}
}

// DoesAutoScrolling
bool
DragSortableListView::DoesAutoScrolling() const
{
	return fScrollPulse;
}

// ScrollTo
void
DragSortableListView::ScrollTo(int32 index)
{
	if (index < 0)
		index = 0;
	if (index >= CountItems())
		index = CountItems() - 1;

	if (ItemAt(index)) {
		BRect itemFrame = ItemFrame(index);
		BRect bounds = Bounds();
		if (itemFrame.top < bounds.top) {
			ScrollTo(itemFrame.LeftTop());
		} else if (itemFrame.bottom > bounds.bottom) {
			ScrollTo(BPoint(0.0, itemFrame.bottom - bounds.Height()));
		}
	}
}

// MoveItems
void
DragSortableListView::MoveItems(const BList& indices, int32 index)
{
	DeselectAll();
	// we remove the items while we look at them, the insertion index is decreased
	// when the items index is lower, so that we insert at the right spot after
	// removal
	BList removedItems;
	int32 count = indices.CountItems();
	for (int32 i = 0; i < count; i++) {
		int32 removeIndex = (int32)indices.ItemAtFast(i) - i;
		BListItem* item = RemoveItem(removeIndex);
		if (item && removedItems.AddItem((void*)item)) {
			if (removeIndex < index)
				index--;
		}
		// else ??? -> blow up
	}
	count = removedItems.CountItems();
	for (int32 i = 0; i < count; i++) {
		BListItem* item = (BListItem*)removedItems.ItemAtFast(i);
		if (AddItem(item, index)) {
			// after we're done, the newly inserted items will be selected
			Select(index, true);
			// next items will be inserted after this one
			index++;
		} else
			delete item;
	}
}

// CopyItems
void
DragSortableListView::CopyItems(const BList& indices, int32 toIndex)
{
	DeselectAll();
	// by inserting the items after we copied all items first, we avoid
	// cloning an item we already inserted and messing everything up
	// in other words, don't touch the list before we know which items
	// need to be cloned
	BList clonedItems;
	int32 count = indices.CountItems();
	for (int32 i = 0; i < count; i++) {
		int32 index = (int32)indices.ItemAtFast(i);
		BListItem* item = CloneItem(index);
		if (item && !clonedItems.AddItem((void*)item))
			delete item;
	}
	count = clonedItems.CountItems();
	for (int32 i = 0; i < count; i++) {
		BListItem* item = (BListItem*)clonedItems.ItemAtFast(i);
		if (AddItem(item, toIndex)) {
			// after we're done, the newly inserted items will be selected
			Select(toIndex, true);
			// next items will be inserted after this one
			toIndex++;
		} else
			delete item;
	}
}

// RemoveItemList
void
DragSortableListView::RemoveItemList(const BList& indices)
{
	int32 count = indices.CountItems();
	for (int32 i = 0; i < count; i++) {
		int32 index = (int32)indices.ItemAtFast(i) - i;
		delete RemoveItem(index);
	}
}

// GetSelectedItems
void
DragSortableListView::GetSelectedItems(BList& indices)
{
	for (int32 i = 0; true; i++) {
		int32 index = CurrentSelection(i);
		if (index < 0)
			break;
		if (!indices.AddItem((void*)index))
			break;
	}
}

// RemoveSelected
void
DragSortableListView::RemoveSelected()
{
	BList indices;
	GetSelectedItems(indices);

	DeselectAll();

	if (indices.CountItems() > 0)
		RemoveItemList(indices);
}

// RemoveAll
void
DragSortableListView::RemoveAll()
{
	BList indices;
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		if (!indices.AddItem((void*)i))
			break;
	}

	if (indices.CountItems() > 0)
		RemoveItemList(indices);
}

// CountSelectedItems
int32
DragSortableListView::CountSelectedItems() const
{
	int32 count = 0;
	while (CurrentSelection(count) >= 0)
		count++;
	return count;
}

// SelectAll
void
DragSortableListView::SelectAll()
{
	Select(0, CountItems() - 1);
}

// DeleteItem
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

// _SetDropAnticipationRect
void
DragSortableListView::_SetDropAnticipationRect(BRect r)
{
	if (fDropRect != r) {
		if (fDropRect.IsValid())
			Invalidate(fDropRect);
		fDropRect = r;
		if (fDropRect.IsValid())
			Invalidate(fDropRect);
	}
}

// _SetDropIndex
void
DragSortableListView::_SetDropIndex(int32 index)
{
	if (fDropIndex != index) {
		fDropIndex = index;
		if (fDropIndex >= 0) {
			int32 count = CountItems();
			if (fDropIndex == count) {
				BRect r;
				if (ItemAt(count - 1)) {
					r = ItemFrame(count - 1);
					r.top = r.bottom;
					r.bottom = r.top + 1.0;
				} else {
					r = Bounds();
					r.bottom--;	// compensate for scrollbars moved slightly out of window
				}
				_SetDropAnticipationRect(r);
			} else {
				BRect r = ItemFrame(fDropIndex);
				r.top--;
				r.bottom = r.top + 1.0;
				_SetDropAnticipationRect(r);
			}
		}
	}
}

// _RemoveDropAnticipationRect
void
DragSortableListView::_RemoveDropAnticipationRect()
{
	_SetDropAnticipationRect(BRect(0.0, 0.0, -1.0, -1.0));
	_SetDropIndex(-1);
}

// _SetDragMessage
void
DragSortableListView::_SetDragMessage(const BMessage* message)
{
	if (message)
		fDragMessageCopy = *message;
	else
		fDragMessageCopy.what = 0;
}



// SimpleListView class
SimpleListView::SimpleListView(BRect frame, BMessage* selectionChangeMessage)
	: DragSortableListView(frame, "playlist listview",
						   B_MULTIPLE_SELECTION_LIST, B_FOLLOW_ALL,
						   B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS),
	  fSelectionChangeMessage(selectionChangeMessage)
{
}

// SimpleListView class
SimpleListView::SimpleListView(BRect frame, const char* name,
							   BMessage* selectionChangeMessage,
							   list_view_type type,
							   uint32 resizingMode, uint32 flags)
	: DragSortableListView(frame, name, type, resizingMode, flags),
	  fSelectionChangeMessage(selectionChangeMessage)
{
}

// destructor
SimpleListView::~SimpleListView()
{
	delete fSelectionChangeMessage;
}

// MessageReceived
void
SimpleListView::MessageReceived( BMessage* message)
{
	switch (message->what) {
		default:
			DragSortableListView::MessageReceived(message);
			break;
	}
}

// SelectionChanged
void
SimpleListView::SelectionChanged()
{
	BLooper* looper = Looper();
	if (fSelectionChangeMessage && looper) {
		BMessage message(*fSelectionChangeMessage);
		looper->PostMessage(&message);
	}
}

// CloneItem
BListItem*
SimpleListView::CloneItem(int32 atIndex) const
{
	BListItem* clone = NULL;
	if (SimpleItem* item = dynamic_cast<SimpleItem*>(ItemAt(atIndex)))
		clone = new SimpleItem(item->Text());
	return clone;
}

// DrawListItem
void
SimpleListView::DrawListItem(BView* owner, int32 index, BRect frame) const
{
	if (SimpleItem* item = dynamic_cast<SimpleItem*>(ItemAt(index))) {
		uint32 flags  = FLAGS_NONE;
		if (index == fFocusedIndex)
			flags |= FLAGS_FOCUSED;
		if (index % 2)
			flags |= FLAGS_TINTED_LINE;
		item->Draw(owner, frame, flags);
	}
}

// MakeDragMessage
void
SimpleListView::MakeDragMessage(BMessage* message) const
{
	if (message) {
		message->AddPointer( "list", (void*)dynamic_cast<const DragSortableListView*>(this));
		int32 index;
		for (int32 i = 0; (index = CurrentSelection(i)) >= 0; i++)
			message->AddInt32( "index", index );
	}
}

