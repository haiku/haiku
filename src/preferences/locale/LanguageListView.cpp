/*
 * Copyright 2006-2010, Haiku.
 * All rights reserved. Distributed under the terms of the MIT License.
 * Authors:
 *		Adrien Destugues <pulkomandy@gmail.com>
 *		Stephan Aßmus <superstippi@gmx.de>
*/
#include "LanguageListView.h"
#include "Locale.h"

#include <Bitmap.h>
#include <Country.h>

#include <stdio.h>

#include <new>


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


//MediaListItem - DrawItem
void 
LanguageListItem::DrawItem(BView *owner, BRect frame, bool complete)
{
	rgb_color kHighlight = { 140,140,140,0 };
	rgb_color kBlack = { 0,0,0,0 };

	BRect r(frame);

	if (IsSelected() || complete) {
		rgb_color color;
		if (IsSelected()) {
			color = kHighlight;
		} else {
			color = owner->ViewColor();
		}
		owner->SetHighColor(color);
		owner->SetLowColor(color);
		owner->FillRect(r);
		owner->SetHighColor(kBlack);
	} else {
		owner->SetLowColor(owner->ViewColor());
	}
	
	frame.left += 4;
	BRect iconFrame(frame);
	iconFrame.Set(iconFrame.left, iconFrame.top+1, iconFrame.left+15, iconFrame.top+16);

	if (fIcon && fIcon->IsValid()) {
		owner->SetDrawingMode(B_OP_OVER);
		owner->DrawBitmap(fIcon, iconFrame);
		owner->SetDrawingMode(B_OP_COPY);
	}

	frame.left += 16 * (OutlineLevel() + 1);
	owner->SetHighColor(kBlack);
	
	BFont		font = be_plain_font;
	font_height	finfo;
	font.GetHeight(&finfo);
	owner->SetFont(&font);
	owner->MovePenTo(frame.left+8, frame.top + ((frame.Height() - (finfo.ascent + finfo.descent + finfo.leading)) / 2) +
					(finfo.ascent + finfo.descent) - 1);
	owner->DrawString(Text());
}



LanguageListView::LanguageListView(const char* name, list_view_type type)
	:
	BOutlineListView(name, type),
	fMsgPrefLanguagesChanged(new BMessage(kMsgPrefLanguagesChanged))
{
}


void
LanguageListView::MoveItems(BList& items, int32 index)
{
	// TODO : only allow moving top level item around other top level
	// or sublevels within the same top level

	DeselectAll();
	// we remove the items while we look at them, the insertion index is
	// decreaded when the items index is lower, so that we insert at the right
	// spot after removal
	BList removedItems;
	int32 count = items.CountItems();
	// We loop in the reverse way so we can remove childs before their parents
	for (int32 i = count - 1; i >= 0; i--) {
		BListItem* item = (BListItem*)items.ItemAt(i);
		int32 removeIndex = IndexOf(item);
		// TODO : remove all childs before removing the item itself, or else
		// they will be lost forever
		if (RemoveItem(item) && removedItems.AddItem((void*)item, 0)) {
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


void LanguageListView::MessageReceived (BMessage* message)
{
	if (message->what == 'DRAG') {
		// Someone just dropped something on us
		LanguageListView* list = NULL;
		if (message->FindPointer("list", (void**)&list) == B_OK) {
			// It comes from a list
			if (list == this) {
				// It comes from ourselves : move the item around in the list
				int32 count = CountItems();
				if (fDropIndex < 0 || fDropIndex > count)
					fDropIndex = count;

				BList items;
				int32 index;
				for (int32 i = 0; message->FindInt32("index", i, &index)
					 	== B_OK; i++)
					if (BListItem* item = FullListItemAt(index))
						items.AddItem((void*)item);

				if (items.CountItems() > 0) {
					// There is something to move
					LanguageListItem* parent =
						static_cast<LanguageListItem*>(Superitem(
							static_cast<LanguageListItem*>(
								items.FirstItem())));
					if (parent) {
						// item has a parent - it should then stay
						// below it
						if (Superitem(FullListItemAt(fDropIndex - 1))
								== parent || FullListItemAt(fDropIndex - 1) == parent)
							MoveItems(items, fDropIndex);
					} else {
						// item is top level and should stay so.
						if (Superitem(FullListItemAt(fDropIndex - 1)) == NULL)
							MoveItems(items, fDropIndex);
						else {
							int itemCount = CountItemsUnder(
								FullListItemAt(fDropIndex), true);
							MoveItems(items, FullListIndexOf(
								Superitem(FullListItemAt(fDropIndex - 1))+itemCount));
						}
					}
				}
				fDropIndex = -1;
			} else {
				// It comes from another list : move it here
				int32 count = CountItems();
				if (fDropIndex < 0 || fDropIndex > count)
					fDropIndex = count;

				// ensure we always drop things at top-level and not
				// in the middle of another outline
				if (Superitem(FullListItemAt(fDropIndex))) {
					// Item has a parent
					fDropIndex = FullListIndexOf(Superitem(FullListItemAt(fDropIndex)));
				}
				
				// Item is now a top level one - we must insert just below its last child
				fDropIndex += CountItemsUnder(FullListItemAt(fDropIndex),false)  + 1;

				int32 index;
				for (int32 i = 0; message->FindInt32("index", i, &index)
						== B_OK; i++) {
					MoveItemFrom(list,index,fDropIndex);
					fDropIndex++;
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
	// If not, we want his parent instead
	LanguageListItem* itemToMove = static_cast<LanguageListItem*>(
		origin->Superitem(origin->FullListItemAt(index)));
	if (itemToMove == NULL) {
		itemToMove = static_cast<LanguageListItem*>(
			origin->FullListItemAt(index));
	} else
		index = origin->FullListIndexOf(itemToMove);

	int itemCount = origin->CountItemsUnder(itemToMove, true);
	LanguageListItem* newItem = new LanguageListItem(*itemToMove);
	this->AddItem(newItem, dropSpot);
	newItem->SetExpanded(itemToMove->IsExpanded());

	for (int i = 0; i < itemCount ; i++) {
		LanguageListItem* subItem = static_cast<LanguageListItem*>(
			origin->ItemUnderAt(itemToMove, true, i));
		this->AddUnder(new LanguageListItem(*subItem),newItem);
	}
	origin->RemoveItem(index);
		// This will also remove the children
}


bool
LanguageListView::InitiateDrag(BPoint point, int32 index, bool)
{
	bool success = false;
	BListItem* item = FullListItemAt(CurrentSelection(0));
	if (!item) {
		// workarround a timing problem
		Select(index);
		item = FullListItemAt(index);
	}
	if (item) {
		// create drag message
		BMessage msg('DRAG');
		msg.AddPointer("list",(void*)(this));
		int32 index;
		for (int32 i = 0; (index = FullListCurrentSelection(i)) >= 0; i++) {
			msg.AddInt32("index", index);
		}
		// figure out drag rect
		float width = Bounds().Width();
		BRect dragRect(0.0, 0.0, width, -1.0);
		// figure out, how many items fit into our bitmap
		int32 numItems;
		bool fade = false;
		for (numItems = 0; BListItem* item = FullListItemAt(CurrentSelection(numItems));
				numItems++) {
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
			BView* v = new BView(dragBitmap->Bounds(), "helper",
				B_FOLLOW_NONE, B_WILL_DRAW);
			dragBitmap->AddChild(v);
			dragBitmap->Lock();
			BRect itemBounds(dragRect) ;
			itemBounds.bottom = 0.0;
			// let all selected items, that fit into our drag_bitmap, draw
			for (int32 i = 0; i < numItems; i++) {
				int32 index = FullListCurrentSelection(i);
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
				for (int32 y = 0; y < height - ALPHA / 2;
						y++, bits += bpr) {
					uint8* line = bits + 3;
					for (uint8* end = line + 4 * width; line < end;
							line += 4)
						*line = ALPHA;
				}
				for (int32 y = height - ALPHA / 2; y < height;
						y++, bits += bpr) {
					uint8* line = bits + 3;
					for (uint8* end = line + 4 * width; line < end;
							line += 4)
						*line = (height - y) << 1;
				}
			} else {
				for (int32 y = 0; y < height; y++, bits += bpr) {
					uint8* line = bits + 3;
					for (uint8* end = line + 4 * width; line < end;
							line += 4)
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
	if (msg && (msg->what == 'DRAG')) {
		switch (transit) {
			case B_ENTERED_VIEW:
			case B_INSIDE_VIEW: {
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
						int32 count = FullListCountItems();
						if (fDropIndex == count) {
							BRect r;
							if (FullListItemAt(count - 1)) {
								r = ItemFrame(count - 1);
								r.top = r.bottom;
								r.bottom = r.top + 1.0;
							} else {
								r = Bounds();
								r.bottom--;
									// compensate for scrollbars moved slightly
									// out of window
							}
						} else {
							BRect r = ItemFrame(fDropIndex);
							r.top--;
							r.bottom = r.top + 1.0;
						}
					}
				}
				break;
			}
		}
	} else
		BOutlineListView::MouseMoved(where, transit, msg);
}
