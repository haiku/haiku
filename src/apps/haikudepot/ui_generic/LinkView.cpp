/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "LinkView.h"

#include <Cursor.h>
#include <String.h>


LinkView::LinkView(const char* name, const char* string, BMessage* message,
	rgb_color color)
	:
	BStringView(name, string),
	BInvoker(message, NULL),
	fNormalColor(color),
	fHoverColor(make_color(1, 141, 211)),
	fEnabled(true),
	fMouseInside(false)
{
	SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	SetExplicitMinSize(BSize(120, B_SIZE_UNSET));
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

	SetLowColor(ViewColor());

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
LinkView::_UpdateLinkColor()
{
	if (fEnabled && fMouseInside) {
		SetHighColor(fHoverColor);
		BCursor cursor(B_CURSOR_ID_FOLLOW_LINK);
		SetViewCursor(&cursor, true);
		Invalidate();
	} else {
		SetHighColor(fNormalColor);
		BCursor cursor(B_CURSOR_SYSTEM_DEFAULT);
		SetViewCursor(&cursor, true);
		Invalidate();
	}
}
