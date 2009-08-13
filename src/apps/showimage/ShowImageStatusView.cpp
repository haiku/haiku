/*
 * Copyright 2003-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fernando Francisco de Oliveira
 *		Michael Wilber
 */

#include "ShowImageStatusView.h"

#include <Entry.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>

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
	ShowImageWindow *window = dynamic_cast<ShowImageWindow *>(Window());
	if (!window || window->GetShowImageView() == NULL)
		return;

	BPath path;
	path.SetTo(window->GetShowImageView()->Image());

	BPopUpMenu popup("no title");
	popup.SetFont(be_plain_font);

	while (path.GetParent(&path) == B_OK && path != "/") {
		popup.AddItem(new BMenuItem(path.Leaf(), NULL));
	}

	BRect bounds(Bounds());
	ConvertToScreen(&bounds);
	where = bounds.LeftBottom();

	BMenuItem *item;
	item = popup.Go(where, true, false, ConvertToScreen(Bounds()));

	if (item) {
		path.SetTo(window->GetShowImageView()->Image());
		path.GetParent(&path);
		int index = popup.IndexOf(item);
		while (index--)
			path.GetParent(&path);
		BMessenger tracker("application/x-vnd.Be-TRAK");
		BMessage msg(B_REFS_RECEIVED);
		entry_ref ref;
		get_ref_for_path(path.Path(), &ref);
		msg.AddRef("refs", &ref);
		tracker.SendMessage(&msg);
	}
}


void
ShowImageStatusView::SetText(BString &text)
{
	fText = text;
	Invalidate();
}

