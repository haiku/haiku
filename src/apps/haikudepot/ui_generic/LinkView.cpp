/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "LinkView.h"

#include <Cursor.h>
#include <String.h>


LinkView::LinkView(const char* name, const char* string, BMessage* message)
	:
	BStringView(name, string),
	BInvoker(message, NULL),
	fEnabled(true),
	fMouseInside(false)
{
	SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	SetExplicitMinSize(BSize(120, B_SIZE_UNSET));
}


void
LinkView::AttachedToWindow()
{
	_UpdateLinkColor();
}


void
LinkView::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	// TODO: Check that no buttons are pressed, don't indicate clickable
	// link if a button is held.
	if (transit == B_ENTERED_VIEW) {
		fMouseInside = true;
		_UpdateLinkColor();
	} else if (transit == B_EXITED_VIEW) {
		fMouseInside = false;
		_UpdateLinkColor();
	}
}


void
LinkView::MouseDown(BPoint where)
{
	if (fEnabled)
		Invoke(Message());
}


void
LinkView::Draw(BRect updateRect)
{
	if (Text() == NULL)
		return;

	SetDrawingMode(B_OP_ALPHA);

	font_height fontHeight;
	GetFontHeight(&fontHeight);

	BRect bounds = Bounds();

	float y = (bounds.top + bounds.bottom - ceilf(fontHeight.ascent)
		- ceilf(fontHeight.descent)) / 2.0 + ceilf(fontHeight.ascent);
	float x = 0.0f;

	BString text(Text());
	TruncateString(&text, B_TRUNCATE_END, bounds.Width());
	DrawString(text, BPoint(x, y));
}


void
LinkView::SetEnabled(bool enabled)
{
	if (fEnabled != enabled) {
		fEnabled = enabled;
		_UpdateLinkColor();
	}
}


void
LinkView::MessageReceived(BMessage* message)
{
	if (message->what == B_COLORS_UPDATED)
		_UpdateLinkColor();

	BStringView::MessageReceived(message);
}


void
LinkView::_UpdateLinkColor()
{
	BCursorID cursorID = B_CURSOR_ID_SYSTEM_DEFAULT;

	float tint = B_DARKEN_1_TINT;
	ViewUIColor(&tint);

	if (fEnabled) {
		if (fMouseInside) {
			cursorID = B_CURSOR_ID_FOLLOW_LINK;
			SetHighUIColor(B_LINK_HOVER_COLOR, tint);
		} else
			SetHighUIColor(B_LINK_TEXT_COLOR, tint);
	} else
		SetHighColor(disable_color(ui_color(B_LINK_TEXT_COLOR), ViewColor()));

	BCursor cursor(cursorID);
	SetViewCursor(&cursor, true);
	Invalidate();
}
