/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "LinkedBitmapView.h"

#include <Cursor.h>


LinkedBitmapView::LinkedBitmapView(const char* name, BMessage* message)
	:
	BitmapView(name),
	BInvoker(message, NULL),
	fEnabled(true),
	fMouseInside(false)
{
}


void
LinkedBitmapView::AllAttached()
{
	//  We don't want to use BitmapView's default behavior here.
}


void
LinkedBitmapView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	// TODO: Check that no buttons are pressed, don't indicate clickable
	// link if a button is held.
	if (transit == B_ENTERED_VIEW) {
		fMouseInside = true;
		_UpdateViewCursor();
	} else if (transit == B_EXITED_VIEW) {
		fMouseInside = false;
		_UpdateViewCursor();
	}
}


void
LinkedBitmapView::MouseDown(BPoint where)
{
	if (fEnabled)
		Invoke(Message());
}


void
LinkedBitmapView::SetEnabled(bool enabled)
{
	if (fEnabled != enabled) {
		fEnabled = enabled;
		_UpdateViewCursor();
	}
}


void
LinkedBitmapView::_UpdateViewCursor()
{
	if (fEnabled && fMouseInside) {
		BCursor cursor(B_CURSOR_ID_FOLLOW_LINK);
		SetViewCursor(&cursor, true);
	} else {
		BCursor cursor(B_CURSOR_SYSTEM_DEFAULT);
		SetViewCursor(&cursor, true);
	}
}
