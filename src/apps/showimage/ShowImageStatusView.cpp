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

#include <Entry.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>

#include <tracker_private.h>
#include "DirMenu.h"

#include "ShowImageView.h"
#include "ShowImageWindow.h"


ShowImageStatusView::ShowImageStatusView(BRect rect, const char* name,
	uint32 resizingMode, uint32 flags)
	:
	BView(rect, name, resizingMode, flags)
{
	SetViewColor(B_TRANSPARENT_32_BIT);
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetHighColor(0, 0, 0, 255);

	BFont font;
	GetFont(&font);
	font.SetSize(10.0);
	SetFont(&font);
}


void
ShowImageStatusView::Draw(BRect updateRect)
{
	rgb_color darkShadow = tint_color(LowColor(), B_DARKEN_2_TINT);
	rgb_color shadow = tint_color(LowColor(), B_DARKEN_1_TINT);
	rgb_color light = tint_color(LowColor(), B_LIGHTEN_MAX_TINT);

	BRect b(Bounds());

	BeginLineArray(5);
		AddLine(BPoint(b.left, b.top),
				BPoint(b.right, b.top), darkShadow);
		b.top += 1.0;
		AddLine(BPoint(b.left, b.top),
				BPoint(b.right, b.top), light);
		AddLine(BPoint(b.right, b.top + 1.0),
				BPoint(b.right, b.bottom), shadow);
		AddLine(BPoint(b.right - 1.0, b.bottom),
				BPoint(b.left + 1.0, b.bottom), shadow);
		AddLine(BPoint(b.left, b.bottom),
				BPoint(b.left, b.top + 1.0), light);
	EndLineArray();

	b.InsetBy(1.0, 1.0);

	// Truncate and layout text
	BString truncated(fText);
	BFont font;
	GetFont(&font);
	font.TruncateString(&truncated, B_TRUNCATE_MIDDLE, b.Width() - 4.0);
	font_height fh;
	font.GetHeight(&fh);

	FillRect(b, B_SOLID_LOW);
	SetDrawingMode(B_OP_OVER);
	DrawString(truncated.String(), BPoint(b.left + 2.0,
		floorf(b.top + b.Height() / 2.0 + fh.ascent / 2.0)));
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
ShowImageStatusView::Update(const entry_ref& ref, const BString& text)
{
	fText = text;
	fRef = ref;

	Invalidate();
}

