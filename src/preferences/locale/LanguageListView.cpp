/*
 * Copyright 2006-2010, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Adrien Destugues <pulkomandy@gmail.com>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include "LanguageListView.h"

#include <stdio.h>

#include <new>

#include <Bitmap.h>
#include <Catalog.h>
#include <FormattingConventions.h>
#include <GradientLinear.h>
#include <LocaleRoster.h>
#include <Region.h>
#include <Window.h>


#define MAX_DRAG_HEIGHT		200.0
#define ALPHA				170

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "LanguageListView"


static const float kLeftInset = 4;

LanguageListItem::LanguageListItem(const char* text, const char* id,
	const char* languageCode)
	:
	BStringItem(text),
	fID(id),
	fCode(languageCode)
{
}


LanguageListItem::LanguageListItem(const LanguageListItem& other)
	:
	BStringItem(other.Text()),
	fID(other.fID),
	fCode(other.fCode)
{
}


void
LanguageListItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	DrawItemWithTextOffset(owner, frame, complete, 0);
}


void
LanguageListItem::DrawItemWithTextOffset(BView* owner, BRect frame,
	bool complete, float textOffset)
{
	if (IsSelected() || complete) {
		rgb_color color;
		if (IsSelected())
			color = ui_color(B_MENU_SELECTED_BACKGROUND_COLOR);
		else
			color = owner->ViewColor();
		owner->SetHighColor(color);
		owner->SetLowColor(color);
		owner->FillRect(frame);
	} else
		owner->SetLowColor(owner->ViewColor());

	BString text = Text();
	if (IsEnabled())
		owner->SetHighColor(ui_color(B_CONTROL_TEXT_COLOR));
	else {
		owner->SetHighColor(tint_color(owner->LowColor(), B_DARKEN_3_TINT));
		text << "   [" << B_TRANSLATE("already chosen") << "]";
	}

	owner->MovePenTo(frame.left + kLeftInset + textOffset,
		frame.top + BaselineOffset());
	owner->DrawString(text.String());
}


// #pragma mark -


LanguageListItemWithFlag::LanguageListItemWithFlag(const char* text,
	const char* id, const char* languageCode, const char* countryCode)
	:
	LanguageListItem(text, id, languageCode),
	fCountryCode(countryCode),
	fIcon(NULL)
{
}


LanguageListItemWithFlag::LanguageListItemWithFlag(
	const LanguageListItemWithFlag& other)
	:
	LanguageListItem(other),
	fCountryCode(other.fCountryCode),
	fIcon(other.fIcon != NULL ? new BBitmap(*other.fIcon) : NULL)
{
}


LanguageListItemWithFlag::~LanguageListItemWithFlag()
{
	delete fIcon;
}


void
LanguageListItemWithFlag::Update(BView* owner, const BFont* font)
{
	LanguageListItem::Update(owner, font);

	float iconSize = Height();
	SetWidth(Width() + iconSize + 4);

	if (fCountryCode.IsEmpty())
		return;

	fIcon = new(std::nothrow) BBitmap(BRect(0, 0, iconSize - 1, iconSize - 1),
		B_RGBA32);
	if (fIcon != NULL && BLocaleRoster::Default()->GetFlagIconForCountry(fIcon,
			fCountryCode.String()) != B_OK) {
		delete fIcon;
		fIcon = NULL;
	}
}


void
LanguageListItemWithFlag::DrawItem(BView* owner, BRect frame, bool complete)
{
	if (fIcon == NULL || !fIcon->IsValid()) {
		DrawItemWithTextOffset(owner, frame, complete, 0);
		return;
	}

	float iconSize = fIcon->Bounds().Width();
	DrawItemWithTextOffset(owner, frame, complete, iconSize + 4);

	BRect iconFrame(frame.left + kLeftInset, frame.top,
		frame.left + kLeftInset + iconSize - 1, frame.top + iconSize - 1);
	owner->SetDrawingMode(B_OP_OVER);
	owner->DrawBitmap(fIcon, iconFrame);
	owner->SetDrawingMode(B_OP_COPY);
}


// #pragma mark -


LanguageListView::LanguageListView(const char* name, list_view_type type)
	:
	BOutlineListView(name, type),
	fDropIndex(-1),
	fDropTargetHighlightFrame(),
	fGlobalDropTargetIndicator(false),
	fDeleteMessage(NULL),
	fDragMessage(NULL)
{
}


LanguageListView::~LanguageListView()
{
}


LanguageListItem*
LanguageListView::ItemForLanguageID(const char* id, int32* _index) const
{
	for (int32 index = 0; index < FullListCountItems(); index++) {
		LanguageListItem* item
			= static_cast<LanguageListItem*>(FullListItemAt(index));

		if (item->ID() == id) {
			if (_index != NULL)
				*_index = index;
			return item;
		}
	}

	return NULL;
}


LanguageListItem*
LanguageListView::ItemForLanguageCode(const char* code, int32* _index) const
{
	for (int32 index = 0; index < FullListCountItems(); index++) {
		LanguageListItem* item
			= static_cast<LanguageListItem*>(FullListItemAt(index));

		if (item->Code() == code) {
			if (_index != NULL)
				*_index = index;
			return item;
		}
	}

	return NULL;
}


void
LanguageListView::SetDeleteMessage(BMessage* message)
{
	delete fDeleteMessage;
	fDeleteMessage = message;
}


void
LanguageListView::SetDragMessage(BMessage* message)
{
	delete fDragMessage;
	fDragMessage = message;
}


void
LanguageListView::SetGlobalDropTargetIndicator(bool isGlobal)
{
	fGlobalDropTargetIndicator = isGlobal;
}


void
LanguageListView::AttachedToWindow()
{
	BOutlineListView::AttachedToWindow();
	ScrollToSelection();
}


void
LanguageListView::MessageReceived(BMessage* message)
{
	if (message->WasDropped() && _AcceptsDragMessage(message)) {
		// Someone just dropped something on us
		BMessage dragMessage(*message);
		dragMessage.AddInt32("drop_index", fDropIndex);
		dragMessage.AddPointer("drop_target", this);
		Messenger().SendMessage(&dragMessage);
	} else
		BOutlineListView::MessageReceived(message);
}


void
LanguageListView::Draw(BRect updateRect)
{
	BOutlineListView::Draw(updateRect);

	if (fDropIndex >= 0 && fDropTargetHighlightFrame.IsValid()) {
		// TODO: decide if drawing of a drop target indicator should be moved
		//       into ControlLook
		BGradientLinear gradient;
		int step = fGlobalDropTargetIndicator ? 64 : 128;
		for (int i = 0; i < 256; i += step)
			gradient.AddColor(i % (step * 2) == 0
				? ViewColor() : ui_color(B_CONTROL_HIGHLIGHT_COLOR), i);
		gradient.AddColor(ViewColor(), 255);
		gradient.SetStart(fDropTargetHighlightFrame.LeftTop());
		gradient.SetEnd(fDropTargetHighlightFrame.RightBottom());
		if (fGlobalDropTargetIndicator) {
			BRegion region(fDropTargetHighlightFrame);
			region.Exclude(fDropTargetHighlightFrame.InsetByCopy(2.0, 2.0));
			ConstrainClippingRegion(&region);
			FillRect(fDropTargetHighlightFrame, gradient);
			ConstrainClippingRegion(NULL);
		} else
			FillRect(fDropTargetHighlightFrame, gradient);
	}
}


bool
LanguageListView::InitiateDrag(BPoint point, int32 dragIndex,
	bool /*wasSelected*/)
{
	if (fDragMessage == NULL)
		return false;

	BListItem* item = ItemAt(CurrentSelection(0));
	if (item == NULL) {
		// workaround for a timing problem
		// TODO: this should support extending the selection
		item = ItemAt(dragIndex);
		Select(dragIndex);
	}
	if (item == NULL)
		return false;

	// create drag message
	BMessage message = *fDragMessage;
	message.AddPointer("listview", this);

	for (int32 i = 0;; i++) {
		int32 index = CurrentSelection(i);
		if (index < 0)
			break;

		message.AddInt32("index", index);
	}

	// figure out drag rect

	BRect dragRect(0.0, 0.0, Bounds().Width(), -1.0);
	int32 numItems = 0;
	bool fade = false;

	// figure out, how many items fit into our bitmap
	for (int32 i = 0, index; message.FindInt32("index", i, &index) == B_OK;
			i++) {
		BListItem* item = ItemAt(index);
		if (item == NULL)
			break;

		dragRect.bottom += ceilf(item->Height()) + 1.0;
		numItems++;

		if (dragRect.Height() > MAX_DRAG_HEIGHT) {
			dragRect.bottom = MAX_DRAG_HEIGHT;
			fade = true;
			break;
		}
	}

	BBitmap* dragBitmap = new BBitmap(dragRect, B_RGB32, true);
	if (dragBitmap->IsValid()) {
		BView* view = new BView(dragBitmap->Bounds(), "helper", B_FOLLOW_NONE,
			B_WILL_DRAW);
		dragBitmap->AddChild(view);
		dragBitmap->Lock();
		BRect itemBounds(dragRect) ;
		itemBounds.bottom = 0.0;
		// let all selected items, that fit into our drag_bitmap, draw
		for (int32 i = 0; i < numItems; i++) {
			int32 index = message.FindInt32("index", i);
			LanguageListItem* item
				= static_cast<LanguageListItem*>(ItemAt(index));
			itemBounds.bottom = itemBounds.top + ceilf(item->Height());
			if (itemBounds.bottom > dragRect.bottom)
				itemBounds.bottom = dragRect.bottom;
			item->DrawItem(view, itemBounds);
			itemBounds.top = itemBounds.bottom + 1.0;
		}
		// make a black frame arround the edge
		view->SetHighColor(0, 0, 0, 255);
		view->StrokeRect(view->Bounds());
		view->Sync();

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

	if (dragBitmap != NULL)
		DragMessage(&message, dragBitmap, B_OP_ALPHA, BPoint(0.0, 0.0));
	else
		DragMessage(&message, dragRect.OffsetToCopy(point), this);

	return true;
}


void
LanguageListView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	if (dragMessage != NULL && _AcceptsDragMessage(dragMessage)) {
		switch (transit) {
			case B_ENTERED_VIEW:
			case B_INSIDE_VIEW:
			{
				BRect highlightFrame;

				if (fGlobalDropTargetIndicator) {
					highlightFrame = Bounds();
					fDropIndex = 0;
				} else {
					// offset where by half of item height
					BRect r = ItemFrame(0);
					where.y += r.Height() / 2.0;

					int32 index = IndexOf(where);
					if (index < 0)
						index = CountItems();
					highlightFrame = ItemFrame(index);
					if (highlightFrame.IsValid())
						highlightFrame.bottom = highlightFrame.top;
					else {
						highlightFrame = ItemFrame(index - 1);
						if (highlightFrame.IsValid())
							highlightFrame.top = highlightFrame.bottom;
						else {
							// empty view, show indicator at top
							highlightFrame = Bounds();
							highlightFrame.bottom = highlightFrame.top;
						}
					}
					fDropIndex = index;
				}

				if (fDropTargetHighlightFrame != highlightFrame) {
					Invalidate(fDropTargetHighlightFrame);
					fDropTargetHighlightFrame = highlightFrame;
					Invalidate(fDropTargetHighlightFrame);
				}

				BOutlineListView::MouseMoved(where, transit, dragMessage);
				return;
			}
		}
	}

	if (fDropTargetHighlightFrame.IsValid()) {
		Invalidate(fDropTargetHighlightFrame);
		fDropTargetHighlightFrame = BRect();
	}
	BOutlineListView::MouseMoved(where, transit, dragMessage);
}


void
LanguageListView::MouseUp(BPoint point)
{
	BOutlineListView::MouseUp(point);
	if (fDropTargetHighlightFrame.IsValid()) {
		Invalidate(fDropTargetHighlightFrame);
		fDropTargetHighlightFrame = BRect();
	}
}


void
LanguageListView::KeyDown(const char* bytes, int32 numBytes)
{
	if (bytes[0] == B_DELETE && fDeleteMessage != NULL) {
		Invoke(fDeleteMessage);
		return;
	}

	BOutlineListView::KeyDown(bytes, numBytes);
}


bool
LanguageListView::_AcceptsDragMessage(const BMessage* message) const
{
	LanguageListView* sourceView = NULL;
	return message != NULL
		&& message->FindPointer("listview", (void**)&sourceView) == B_OK;
}
