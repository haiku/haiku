/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "InputTextView.h"

#include <stdio.h>
#include <stdlib.h>

#include <String.h>

// constructor
InputTextView::InputTextView(BRect frame, const char* name,
							 BRect textRect,
							 uint32 resizingMode,
							 uint32 flags)
	: BTextView(frame, name, textRect, resizingMode, flags),
	  fWasFocus(false),
	  fTextBeforeFocus("")
{
	SetWordWrap(false);
}

// destructor
InputTextView::~InputTextView()
{
}

// MouseDown
void
InputTextView::MouseDown(BPoint where)
{
	// enforce the behaviour of a typical BTextControl
	// only let the BTextView handle mouse up/down when
	// it already had focus
	fWasFocus = IsFocus();
	if (fWasFocus) {
		BTextView::MouseDown(where);
	} else {
		// forward click
		if (BView* view = Parent()) {
			view->MouseDown(ConvertToParent(where));
		}
	}
}

// MouseUp
void
InputTextView::MouseUp(BPoint where)
{
	// enforce the behaviour of a typical BTextControl
	// only let the BTextView handle mouse up/down when
	// it already had focus
	if (fWasFocus)
		BTextView::MouseUp(where);
}

// KeyDown
void
InputTextView::KeyDown(const char* bytes, int32 numBytes)
{
	bool handled = true;
	if (numBytes > 0) {
		switch (bytes[0]) {
			case B_ESCAPE:
				// revert any typing changes
				RevertChanges();
				fTextBeforeFocus = Text();
				break;
			case B_RETURN:
				ApplyChanges();
				fTextBeforeFocus = Text();
				break;
			default:
				handled = false;
				break;
		}
	}
	if (!handled)
		BTextView::KeyDown(bytes, numBytes);
}

// MakeFocus
void
InputTextView::MakeFocus(bool focus)
{
	if (focus == IsFocus())
		return;

	if (BView* view = Parent())
		view->Invalidate();
	BTextView::MakeFocus(focus);
	if (focus) {
		SelectAll();
		fTextBeforeFocus = Text();
	} else {
		// Only pressing ESC is supposed to revert to the previous
		// value and not invoke, but in that case, the text will already
		// be the previous value when we lose focus here.
		if (fTextBeforeFocus != Text())
			Invoke();
	}
}

// Invoke
status_t
InputTextView::Invoke(BMessage* message)
{
	if (!message)
		message = Message();

	if (message) {
		BMessage copy(*message);
		copy.AddInt64("when", system_time());
		copy.AddPointer("source", (BView*)this);
		return BInvoker::Invoke(&copy);
	}
	return B_BAD_VALUE;
}
