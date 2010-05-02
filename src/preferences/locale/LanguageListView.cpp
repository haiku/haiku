/*
 * Copyright 2006-2010, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Adrien Destugues <pulkomandy@gmail.com>
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include "LanguageListView.h"

#include <stdio.h>

#include <new>

#include <Bitmap.h>
#include <Country.h>

#include "Locale.h"


#define MAX_DRAG_HEIGHT		200.0
#define ALPHA				170
#define TEXT_OFFSET			5.0


LanguageListItem::LanguageListItem(const char* text, const char* code)
	:
	BStringItem(text),
	fLanguageCode(code)
{
	// TODO: should probably keep the BCountry as a member of the class
	BCountry myCountry(code);
	BRect bounds(0, 0, 15, 15);
	fIcon = new(std::nothrow) BBitmap(bounds, B_RGBA32);
	if (fIcon && myCountry.GetIcon(fIcon) != B_OK) {
		delete fIcon;
		fIcon = NULL;
	}
}


LanguageListItem::~LanguageListItem()
{
	delete fIcon;
}


void
LanguageListItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	rgb_color kHighlight = { 140,140,140,0 };
	rgb_color kBlack = { 0,0,0,0 };

	BRect r(frame);

	if (IsSelected() || complete) {
		rgb_color color;
		if (IsSelected())
			color = kHighlight;
		else
			color = owner->ViewColor();
		owner->SetHighColor(color);
		owner->SetLowColor(color);
		owner->FillRect(r);
		owner->SetHighColor(kBlack);
	} else
		owner->SetLowColor(owner->ViewColor());

	frame.left += 4;
	BRect iconFrame(frame);
	iconFrame.Set(iconFrame.left, iconFrame.top+1, iconFrame.left+15,
		iconFrame.top+16);

	if (fIcon && fIcon->IsValid()) {
		owner->SetDrawingMode(B_OP_OVER);
		owner->DrawBitmap(fIcon, iconFrame);
		owner->SetDrawingMode(B_OP_COPY);
	}

	frame.left += 16 * (OutlineLevel() + 1);
	owner->SetHighColor(kBlack);

	BFont font = be_plain_font;
	font_height	finfo;
	font.GetHeight(&finfo);
	owner->SetFont(&font);
	owner->MovePenTo(frame.left+8, frame.top + ((frame.Height() - (finfo.ascent
					+ finfo.descent + finfo.leading)) / 2) + (finfo.ascent
			+ finfo.descent) - 1);
	owner->DrawString(Text());
}


// #pragma mark -


LanguageListView::LanguageListView(const char* name, list_view_type type)
	:
	BOutlineListView(name, type),
	fMsgPrefLanguagesChanged(new BMessage(kMsgPrefLanguagesChanged))
{
}


LanguageListView::~LanguageListView()
{
	delete fMsgPrefLanguagesChanged;
}


void
LanguageListView::AttachedToWindow()
{
	BOutlineListView::AttachedToWindow();
	ScrollToSelection();
}


void
LanguageListView::MoveItems(BList& items, int32 index)
{
	// TODO : only allow moving top level item around other top level
	// or sublevels within the same top level

	DeselectAll();

	// collect all items that must be moved (adding subitems as necessary)
	int32 count = items.CountItems();
	BList itemsToBeMoved;
	for (int32 i = 0; i < count; i++) {
		BListItem* item = (BListItem*)items.ItemAt(i);
		itemsToBeMoved.AddItem(item);
		if (item->OutlineLevel() == 0) {
			// add all subitems, as they need to be moved, too
			int32 subItemCount = CountItemsUnder(item, true);
			for (int subIndex = 0; subIndex < subItemCount ; subIndex++)
				itemsToBeMoved.AddItem(ItemUnderAt(item, true, subIndex));
		}
	}

	// now remove all the items backwards (in order to remove children before
	// their parent), decreasing target index if we are removing items before
	// it
	count = itemsToBeMoved.CountItems();
	for (int32 i = count - 1; i >= 0; i--) {
		BListItem* item = (BListItem*)itemsToBeMoved.ItemAt(i);
		int32 removeIndex = FullListIndexOf(item);
		if (RemoveItem(item)) {
			if (removeIndex < index)
				index--;
		}
	}

	// finally add all the items at the given index
	for (int32 i = 0; i < count; i++) {
		BListItem* item = (BListItem*)itemsToBeMoved.ItemAt(i);
		if (AddItem(item, index))
			index++;
		else
			delete item;
	}
}


void
LanguageListView::MessageReceived(BMessage* message)
{
	if (message->what == 'DRAG') {
		// Someone just dropped something on us
		LanguageListView* list = NULL;
		if (message->FindPointer("list", (void**)&list) == B_OK) {
			// It comes from a list
			int32 count = CountItems();
			if (fDropIndex < 0 || fDropIndex > count)
				fDropIndex = count;

			if (list == this) {
				// It comes from ourselves : move the item around in the list
				BList items;
				int32 index;
				for (int32 i = 0;
					message->FindInt32("index", i, &index) == B_OK; i++) {
					if (BListItem* item = FullListItemAt(index))
						items.AddItem((void*)item);
				}

				if (items.CountItems() > 0) {
					// There is something to move
					LanguageListItem* parent = static_cast<LanguageListItem*>(
						Superitem(static_cast<LanguageListItem*>(
								items.FirstItem())));
					if (parent) {
						// item has a parent - it should then stay
						// below it
						if (Superitem(FullListItemAt(fDropIndex - 1)) == parent
							|| FullListItemAt(fDropIndex - 1) == parent)
							MoveItems(items, fDropIndex);
					} else {
						// item is top level and should stay so.
						if (Superitem(FullListItemAt(fDropIndex - 1)) == NULL)
							MoveItems(items, fDropIndex);
						else {
							int itemCount = CountItemsUnder(
								FullListItemAt(fDropIndex), true);
							MoveItems(items, FullListIndexOf(Superitem(
										FullListItemAt(fDropIndex - 1))
									+ itemCount));
						}
					}
				}
				fDropIndex = -1;
			} else {
				// It comes from another list : move it here

				// ensure we always drop things at top-level and not
				// in the middle of another outline
				if (Superitem(FullListItemAt(fDropIndex))) {
					// Item has a parent
					fDropIndex = FullListIndexOf(Superitem(FullListItemAt(
								fDropIndex)));
				}

				// Item is now a top level one - we must insert just below its
				// last child
				fDropIndex += CountItemsUnder(FullListItemAt(fDropIndex), true)
					+ 1;

				int32 indexCount;
				type_code dummy;
				if (message->GetInfo("index", &dummy, &indexCount) == B_OK) {
					for (int32 i = indexCount - 1; i >= 0; i--) {
						int32 index;
						if (message->FindInt32("index", i, &index) == B_OK)
							MoveItemFrom(list, index, fDropIndex);
					}
				}

				fDropIndex = -1;
			}
			Invoke(fMsgPrefLanguagesChanged);
		}
	} else
		BOutlineListView::MessageReceived(message);
}


void
LanguageListView::MoveItemFrom(BOutlineListView* origin, int32 index,
	int32 dropSpot)
{
	// Check that the node we are going to move is a top-level one.
	// If not, we want its parent instead
	LanguageListItem* itemToMove = static_cast<LanguageListItem*>(
		origin->Superitem(origin->FullListItemAt(index)));
	if (itemToMove == NULL) {
		itemToMove = static_cast<LanguageListItem*>(
			origin->FullListItemAt(index));
		if (itemToMove == NULL)
			return;
	} else
		index = origin->FullListIndexOf(itemToMove);

	// collect all items that must be moved (adding subitems as necessary)
	BList itemsToBeMoved;
	itemsToBeMoved.AddItem(itemToMove);
	// add all subitems, as they need to be moved, too
	int32 subItemCount = origin->CountItemsUnder(itemToMove, true);
	for (int32 subIndex = 0; subIndex < subItemCount; subIndex++) {
		itemsToBeMoved.AddItem(origin->ItemUnderAt(itemToMove, true,
			subIndex));
	}

	// now remove all items from origin in reverse order (to remove the children
	// before the parent) ...
	// TODO: using RemoveItem() on the parent will delete the subitems, which
	//       may be a bug, actually.
	int32 itemCount = itemsToBeMoved.CountItems();
	for (int32 i = itemCount - 1; i >= 0; i--)
		origin->RemoveItem(index + i);
	// ... and add all the items to this list
	AddList(&itemsToBeMoved, dropSpot);
}


bool
LanguageListView::InitiateDrag(BPoint point, int32 index, bool)
{
	bool success = false;
	BListItem* item = FullListItemAt(CurrentSelection(0));
	if (!item) {
		// workaround for a timing problem
		Select(index);
		item = FullListItemAt(index);
	}
	if (item) {
		// create drag message
		BMessage msg('DRAG');
		msg.AddPointer("list", (void*)(this));
		// first selection round, consider only superitems
		int32 index;
		for (int32 i = 0; (index = FullListCurrentSelection(i)) >= 0; i++) {
			BListItem* item = FullListItemAt(index);
			if (item == NULL)
				return false;
			if (item->OutlineLevel() == 0)
				msg.AddInt32("index", index);
		}
		if (!msg.HasInt32("index")) {
			// second selection round, consider only subitems of the same
			// (i.e. the first) parent
			BListItem* seenSuperItem = NULL;
			for (int32 i = 0; (index = FullListCurrentSelection(i)) >= 0; i++) {
				BListItem* item = FullListItemAt(index);
				if (item == NULL)
					return false;
				if (item->OutlineLevel() != 0) {
					BListItem* superItem = Superitem(item);
					if (seenSuperItem == NULL)
						seenSuperItem = superItem;
					if (superItem == seenSuperItem)
						msg.AddInt32("index", index);
					else
						break;
				}
			}
		}

		// figure out drag rect
		float width = Bounds().Width();
		BRect dragRect(0.0, 0.0, width, -1.0);
		// figure out, how many items fit into our bitmap
		bool fade = false;
		int32 numItems;
		int32 currIndex;
		BListItem* item;
		for (numItems = 0;
			msg.FindInt32("index", numItems, &currIndex) == B_OK
				&& (item = FullListItemAt(currIndex)) != NULL; numItems++) {
			dragRect.bottom += ceilf(item->Height()) + 1.0;
			if (dragRect.Height() > MAX_DRAG_HEIGHT) {
				fade = true;
				dragRect.bottom = MAX_DRAG_HEIGHT;
				numItems++;
				break;
			}
		}
		BBitmap* dragBitmap = new BBitmap(dragRect, B_RGB32, true);
		if (dragBitmap && dragBitmap->IsValid()) {
			BView* v = new BView(dragBitmap->Bounds(), "helper", B_FOLLOW_NONE,
				B_WILL_DRAW);
			dragBitmap->AddChild(v);
			dragBitmap->Lock();
			BRect itemBounds(dragRect) ;
			itemBounds.bottom = 0.0;
			// let all selected items, that fit into our drag_bitmap, draw
			for (int32 i = 0; i < numItems; i++) {
				int32 index = msg.FindInt32("index", i);
				LanguageListItem* item
					= static_cast<LanguageListItem*>(FullListItemAt(index));
				itemBounds.bottom = itemBounds.top + ceilf(item->Height());
				if (itemBounds.bottom > dragRect.bottom)
					itemBounds.bottom = dragRect.bottom;
				item->DrawItem(v, itemBounds);
				itemBounds.top = itemBounds.bottom + 1.0;
			}
			// make a black frame arround the edge
			v->SetHighColor(0, 0, 0, 255);
			v->StrokeRect(v->Bounds());
			v->Sync();

			uint8* bits = (uint8*)dragBitmap->Bits();
			int32 height = (int32)dragBitmap->Bounds().Height() + 1;
			int32 width = (int32)dragBitmap->Bounds().Width() + 1;
			int32 bpr = dragBitmap->BytesPerRow();

			if (fade) {
				for (int32 y = 0; y < height - ALPHA / 2; y++, bits += bpr) {
					uint8* line = bits + 3;
					for (uint8* end = line + 4 * width; line < end; line += 4)
						*line = ALPHA;
				}
				for (int32 y = height - ALPHA / 2; y < height;
					y++, bits += bpr) {
					uint8* line = bits + 3;
					for (uint8* end = line + 4 * width; line < end; line += 4)
						*line = (height - y) << 1;
				}
			} else {
				for (int32 y = 0; y < height; y++, bits += bpr) {
					uint8* line = bits + 3;
					for (uint8* end = line + 4 * width; line < end; line += 4)
						*line = ALPHA;
				}
			}
			dragBitmap->Unlock();
		} else {
			delete dragBitmap;
			dragBitmap = NULL;
		}
		if (dragBitmap)
			DragMessage(&msg, dragBitmap, B_OP_ALPHA, BPoint(0.0, 0.0));
		else
			DragMessage(&msg, dragRect.OffsetToCopy(point), this);

		success = true;
	}
	return success;
}


void
LanguageListView::MouseMoved(BPoint where, uint32 transit, const BMessage* msg)
{
	if (msg && msg->what == 'DRAG') {
		switch (transit) {
			case B_ENTERED_VIEW:
			case B_INSIDE_VIEW:
			{
				// set drop target through virtual function
				// offset where by half of item height
				BRect r = ItemFrame(0);
				where.y += r.Height() / 2.0;

				int32 index = FullListIndexOf(where);
				if (index < 0)
					index = FullListCountItems();
				if (fDropIndex != index) {
					fDropIndex = index;
					if (fDropIndex >= 0) {
// TODO: find out what this was intended for (as it doesn't have any effect)
//						int32 count = FullListCountItems();
//						if (fDropIndex == count) {
//							BRect r;
//							if (FullListItemAt(count - 1)) {
//								r = ItemFrame(count - 1);
//								r.top = r.bottom;
//								r.bottom = r.top + 1.0;
//							} else {
//								r = Bounds();
//								r.bottom--;
//									// compensate for scrollbars moved slightly
//									// out of window
//							}
//						} else {
//							BRect r = ItemFrame(fDropIndex);
//							r.top--;
//							r.bottom = r.top + 1.0;
//						}
					}
				}
				break;
			}
		}
	} else
		BOutlineListView::MouseMoved(where, transit, msg);
}
