/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "StringTextView.h"

#include <stdio.h>
#include <stdlib.h>

// constructor
StringTextView::StringTextView(BRect frame, const char* name,
							   BRect textRect,
							   uint32 resizingMode,
							   uint32 flags)
	: InputTextView(frame, name, textRect, resizingMode, flags),
	  fStringCache("")
{
}

// destructor
StringTextView::~StringTextView()
{
}

// Invoke
status_t
StringTextView::Invoke(BMessage* message)
{
	if (!message)
		message = Message();

	if (message) {
		BMessage copy(*message);
		copy.AddString("value", Value());
		return InputTextView::Invoke(&copy);
	}
	return B_BAD_VALUE;
}

// RevertChanges
void
StringTextView::RevertChanges()
{
	SetValue(fStringCache.String());
}

// ApplyChanges
void
StringTextView::ApplyChanges()
{
	if (fStringCache != Text()) {
		Invoke();
	}
}

// SetValue
void
StringTextView::SetValue(const char* string)
{
	SetText(string);

	// update cache
	Value();

	if (IsFocus())
		SelectAll();
}

// Value
const char*
StringTextView::Value() const
{
	fStringCache = Text();
	return fStringCache.String();
}

