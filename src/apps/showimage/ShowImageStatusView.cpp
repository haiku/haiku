/*
 * Copyright 2003-2010, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fernando Francisco de Oliveira
 *		Michael Wilber
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "ShowImageStatusView.h"

#include <ControlLook.h>
#include <Entry.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <ScrollView.h>

#include <tracker_private.h>
#include "DirMenu.h"

#include "ShowImageView.h"
#include "ShowImageWindow.h"

const float kHorzSpacing = 5.f;


ShowImageStatusView::ShowImageStatusView(BScrollView* scrollView)
	:
	BView(BRect(), "statusview", B_FOLLOW_BOTTOM | B_FOLLOW_LEFT, B_WILL_DRAW),
	fScrollView(scrollView),
	fPreferredSize(0.0, 0.0)
{
	memset(fCellWidth, 0, sizeof(fCellWidth));
}


void
ShowImageStatusView::AttachedToWindow()
{
	SetFont(be_plain_font);
	SetFontSize(10.0);

	BScrollBar* scrollBar = fScrollView->ScrollBar(B_HORIZONTAL);
	MoveTo(0.0, scrollBar->Frame().top);

	AdoptParentColors();

	ResizeToPreferred();
}


void
ShowImageStatusView::GetPreferredSize(float* _width, float* _height)
{
	_ValidatePreferredSize();

	if (_width)
		*_width = fPreferredSize.width;

	if (_height)
		*_height = fPreferredSize.height;
}


void
ShowImageStatusView::ResizeToPreferred()
{
	float width, height;
	GetPreferredSize(&width, &height);

	if (Bounds().Width() > width)
		width = Bounds().Width();

	BView::ResizeTo(width, height);
}


void
ShowImageStatusView::Draw(BRect updateRect)
{
	if (fPreferredSize.width <= 0)
		return;

	if (be_control_look != NULL) {
		BRect bounds(Bounds());
		be_control_look->DrawMenuBarBackground(this,
			bounds, updateRect,	ViewColor());
	}

	BRect bounds(Bounds());
	rgb_color highColor = ui_color(B_PANEL_TEXT_COLOR);

	SetHighColor(tint_color(ViewColor(), B_DARKEN_2_TINT));
	StrokeLine(bounds.LeftTop(), bounds.RightTop());

	float x = bounds.left;
	for (size_t i = 0; i < kStatusCellCount - 1; i++) {
		x += fCellWidth[i];
		StrokeLine(BPoint(x, bounds.top + 3), BPoint(x, bounds.bottom - 3));
	}

	SetLowColor(ViewColor());
	SetHighColor(highColor);

	font_height fontHeight;
	GetFontHeight(&fontHeight);

	x = bounds.left;
	float y = (bounds.bottom + bounds.top
		+ ceilf(fontHeight.ascent) - ceilf(fontHeight.descent)) / 2;

	for (size_t i = 0; i < kStatusCellCount; i++) {
		if (fCellText[i].Length() == 0)
			continue;
		DrawString(fCellText[i], BPoint(x + kHorzSpacing, y));
		x += fCellWidth[i];
	}
}


void
ShowImageStatusView::MouseDown(BPoint where)
{
	BPrivate::BDirMenu* menu = new BDirMenu(NULL, BMessenger(kTrackerSignature),
		B_REFS_RECEIVED);
	BEntry entry;
	if (entry.SetTo(&fRef) == B_OK)
		menu->Populate(&entry, Window(), false, false, true, false, true);
	else
		menu->Populate(NULL, Window(), false, false, true, false, true);

	BPoint point = Bounds().LeftBottom();
	point.y += 3;
	ConvertToScreen(&point);
	BRect clickToOpenRect(Bounds());
	ConvertToScreen(&clickToOpenRect);
	menu->Go(point, true, true, clickToOpenRect);
	delete menu;
}


void
ShowImageStatusView::Update(const entry_ref& ref, const BString& text,
	const BString& pages, const BString& imageType, float zoom)
{
	fRef = ref;

	_SetFrameText(text);
	_SetZoomText(zoom);
	_SetPagesText(pages);
	_SetImageTypeText(imageType);

	_ValidatePreferredSize();
	Invalidate();
}


void
ShowImageStatusView::SetZoom(float zoom)
{
	_SetZoomText(zoom);
	_ValidatePreferredSize();
	Invalidate();
}


void
ShowImageStatusView::_SetFrameText(const BString& text)
{
	fCellText[kFrameSizeCell] = text;
}


void
ShowImageStatusView::_SetZoomText(float zoom)
{
	fCellText[kZoomCell].SetToFormat("%.0f%%", zoom * 100);
}


void
ShowImageStatusView::_SetPagesText(const BString& pages)
{
	fCellText[kPagesCell] = pages;
}


void
ShowImageStatusView::_SetImageTypeText(const BString& imageType)
{
	fCellText[kImageTypeCell] = imageType;
}


void
ShowImageStatusView::_ValidatePreferredSize()
{
	float orgWidth = fPreferredSize.width;
	// width
	fPreferredSize.width = 0.f;
	for (size_t i = 0; i < kStatusCellCount; i++) {
		if (fCellText[i].Length() == 0) {
			fCellWidth[i] = 0;
			continue;
		}
		float width = ceilf(StringWidth(fCellText[i]));
		if (width > 0)
			width += kHorzSpacing * 2;
		fCellWidth[i] = width;
		fPreferredSize.width += fCellWidth[i];
	}

	// height
	font_height fontHeight;
	GetFontHeight(&fontHeight);

	fPreferredSize.height = ceilf(fontHeight.ascent + fontHeight.descent
		+ fontHeight.leading);

	float scrollBarSize = be_control_look->GetScrollBarWidth(B_HORIZONTAL);
	if (fPreferredSize.height < scrollBarSize)
		fPreferredSize.height = scrollBarSize;

	float delta = fPreferredSize.width - orgWidth;
	ResizeBy(delta, 0);
	BScrollBar* scrollBar = fScrollView->ScrollBar(B_HORIZONTAL);
	scrollBar->ResizeBy(-delta, 0);
	scrollBar->MoveBy(delta, 0);
}
