/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 */

#include <stdio.h>

#include <Message.h>
#include <Bitmap.h>
#include <Window.h>
#include <List.h>

#include "IconRule.h"
#include "IconItem.h"

const int32 kEdgeOffset = 8;
const int32 kBorderOffset = 1;

// TODO: Do we need to inherit from BControl?

BIconRule::BIconRule(const char* name)
	:
	BView(name, B_WILL_DRAW),
	fSelIndex(-1)
{
	fIcons = new BList();
}


BIconRule::~BIconRule()
{
	delete fIcons;
}


status_t
BIconRule::Invoke(BMessage* message)
{
	bool notify = false;
	uint32 kind = InvokeKind(&notify);

	BMessage clone(kind);
	status_t err = B_BAD_VALUE;

	if (!message && !notify)
		message = Message();

	if (!message) {
		if (!IsWatched())
			return err;
	} else
		clone = *message;

	clone.AddInt64("when", (int64)system_time());
	clone.AddPointer("source", this);
	clone.AddMessenger("be:sender", BMessenger(this));
	clone.AddInt32("index", fSelIndex);

	if (message)
		err = BInvoker::Invoke(&clone);

	SendNotices(kind, &clone);

	return err;
}


void
BIconRule::AttachedToWindow()
{
	BView::AttachedToWindow();

	if (!Messenger().IsValid())
		SetTarget(Window(), NULL);
}


void
BIconRule::MouseDown(BPoint point)
{
	if (!IsFocus()) {
		MakeFocus();
		Sync();
		Window()->UpdateIfNeeded();
	}

	int32 index = IndexOf(point);
	if (index > -1)
		SlideToIcon(index);
}


void
BIconRule::Draw(BRect updateRect)
{
	int32 count = CountIcons();
	if (count == 0)
		return;

	rgb_color panelColor = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color lightColor = tint_color(panelColor, B_DARKEN_1_TINT);
	rgb_color darkColor = tint_color(lightColor, B_DARKEN_2_TINT);

	SetHighColor(darkColor);
	StrokeLine(Bounds().LeftTop(), Bounds().RightTop());
	StrokeLine(Bounds().LeftTop(), Bounds().LeftBottom());

	SetHighColor(lightColor);
	StrokeLine(Bounds().LeftBottom(), Bounds().RightBottom());
	StrokeLine(Bounds().RightTop(), Bounds().RightBottom());

	BRect itemFrame(kEdgeOffset, kBorderOffset, -1, kBorderOffset + 64);
	for (int32 i = 0; i < count; i++) {
		BIconItem* item = static_cast<BIconItem*>(fIcons->ItemAt(i));
		float width = StringWidth(item->Label()) + StringWidth(" ") * 2;
		if (width < 64.0f)
			width = 64.0f;
		itemFrame.right = itemFrame.left + width - 1;

		if (itemFrame.Intersects(updateRect)) {
			item->SetFrame(itemFrame);
			item->Draw();
		}

		itemFrame.left = itemFrame.right + kEdgeOffset + 1;
	}
}


BMessage*
BIconRule::SelectionMessage() const
{
	return fMessage;
}


void
BIconRule::SetSelectionMessage(BMessage* message)
{
	delete fMessage;
	fMessage = message;
}


void
BIconRule::AddIcon(const char* label, const BBitmap* icon)
{
	BIconItem* item = new BIconItem(this, label, (BBitmap*)icon);
	if (CountIcons() == 0) {
		item->Select();
		fSelIndex = 0;
	}
	(void)fIcons->AddItem(item);
}


void
BIconRule::RemoveIconAt(int32 index)
{
}


void
BIconRule::RemoveAllIcons()
{
}


int32
BIconRule::CountIcons() const
{
	return fIcons->CountItems();
}


void
BIconRule::SlideToIcon(int32 index)
{
	// Ignore invalid items
	if ((index < 0) || (index > CountIcons() - 1))
		return;

	BIconItem* item = static_cast<BIconItem*>(fIcons->ItemAt(index));
	if (item) {
		// Deselect previously selected item
		if (fSelIndex > -1) {
			BIconItem* selItem = static_cast<BIconItem*>(fIcons->ItemAt(fSelIndex));
			selItem->Deselect();
		}

		// Select this item
		item->Select();
		fSelIndex = index;
		Invalidate();

		// Invoke notification
		InvokeNotify(fMessage, B_CONTROL_MODIFIED);
	}
}


void
BIconRule::SlideToNext()
{
	if (fSelIndex + 1 < CountIcons() - 1)
		return;
	SlideToIcon(fSelIndex + 1);
}


void
BIconRule::SlideToPrevious()
{
	if (fSelIndex <= 0)
		return;
	SlideToIcon(fSelIndex - 1);
}


int32
BIconRule::IndexOf(BPoint point)
{
	int32 low = 0;
	int32 high = fIcons->CountItems() - 1;
	int32 mid = -1;
	float frameLeft = -1.0f;
	float frameRight = 1.0f;

	// Binary search the list
	while (high >= low) {
		mid = (low + high) / 2;
		BIconItem* item = static_cast<BIconItem*>(fIcons->ItemAt(mid));
		frameLeft = item->Frame().left;
		frameRight = item->Frame().right;
		if (point.x < frameLeft)
			high = mid - 1;
		else if (point.x > frameRight)
			low = mid + 1;
		else
			return mid;
	}

	return -1;
}

BSize
BIconRule::MinSize()
{
	BSize minSize(BView::MinSize());
	minSize.height = 64 + (kBorderOffset * 2);
	return minSize;
}


BSize
BIconRule::MaxSize()
{
	return BView::MaxSize();
}


BSize
BIconRule::PreferredSize()
{
	return BView::PreferredSize();
}
