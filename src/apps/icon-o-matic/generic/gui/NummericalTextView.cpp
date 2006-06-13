/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "NummericalTextView.h"

#include <stdio.h>
#include <stdlib.h>

#include <String.h>

// constructor
NummericalTextView::NummericalTextView(BRect frame, const char* name,
									   BRect textRect,
									   uint32 resizingMode,
									   uint32 flags)
	: InputTextView(frame, name, textRect, resizingMode, flags)
{
	for (uint32 i = 0; i < '0'; i++) {
		DisallowChar(i);
	}
	for (uint32 i = '9' + 1; i < 255; i++) {
		DisallowChar(i);
	}
	AllowChar('-');
}

// destructor
NummericalTextView::~NummericalTextView()
{
}

// Invoke
status_t
NummericalTextView::Invoke(BMessage* message)
{
	if (!message)
		message = Message();

	if (message) {
		BMessage copy(*message);
		copy.AddInt32("be:value", IntValue());
		copy.AddFloat("float value", FloatValue());
		return InputTextView::Invoke(&copy);
	}
	return B_BAD_VALUE;
}

// RevertChanges
void
NummericalTextView::RevertChanges()
{
	if (fFloatMode)
		SetValue(fFloatValueCache);
	else
		SetValue(fIntValueCache);
}

// ApplyChanges
void
NummericalTextView::ApplyChanges()
{
	int32 i = atoi(Text());
	float f = atof(Text());

	if ((fFloatMode && f != fFloatValueCache) ||
		(!fFloatMode && i != fIntValueCache)) {
		Invoke();
	}
}

// SetFloatMode
void
NummericalTextView::SetFloatMode(bool floatingPoint)
{
	fFloatMode = floatingPoint;
	if (floatingPoint)
		AllowChar('.');
	else
		DisallowChar('.');
}

// SetValue
void
NummericalTextView::SetValue(int32 value)
{
	BString helper;
	helper << value;
	SetText(helper.String());

	// update caches
	IntValue();
	FloatValue();

	if (IsFocus())
		SelectAll();
}

// SetValue
void
NummericalTextView::SetValue(float value)
{
	BString helper;
	helper << value;
	SetText(helper.String());

	// update caches
	IntValue();
	FloatValue();

	if (IsFocus())
		SelectAll();
}

// IntValue
int32
NummericalTextView::IntValue() const
{
	fIntValueCache = atoi(Text());
	return fIntValueCache;
}

// FloatValue
float
NummericalTextView::FloatValue() const
{
	fFloatValueCache = atof(Text());
	return fFloatValueCache;
}

// #pragma mark -

// Select
void
NummericalTextView::Select(int32 start, int32 finish)
{
	InputTextView::Select(start, finish);

	_CheckMinusAllowed();
	_CheckDotAllowed();
}

// InsertText
void
NummericalTextView::InsertText(const char* inText, int32 inLength, int32 inOffset,
							   const text_run_array* inRuns)
{
	InputTextView::InsertText(inText, inLength, inOffset, inRuns);

	_CheckMinusAllowed();
	_CheckDotAllowed();
}

// DeleteText
void
NummericalTextView::DeleteText(int32 fromOffset, int32 toOffset)
{
	InputTextView::DeleteText(fromOffset, toOffset);

	_CheckMinusAllowed();
	_CheckDotAllowed();
}

// #pragma mark -

// _ToggleAllowChar
void
NummericalTextView::_ToggleAllowChar(char c)
{
	const char* text = Text();
	if (text) {
		bool found = false;
		int32 selectionStart;
		int32 selectionEnd;
		GetSelection(&selectionStart, &selectionEnd);
		int32 pos = 0;
		while (text[pos]) {
			// skip selection
			if (selectionStart < selectionEnd
				&& pos == selectionStart) {
				pos = selectionEnd;
			}
			if (text[pos] == c) {
				found = true;
				break;
			}
			pos++;
		}
		if (found)
			DisallowChar(c);
		else
			AllowChar(c);
	}
}

// _CheckMinusAllowed
void
NummericalTextView::_CheckMinusAllowed()
{
	_ToggleAllowChar('-');
}

// _CheckDotAllowed
void
NummericalTextView::_CheckDotAllowed()
{
	if (fFloatMode) {
		_ToggleAllowChar('.');
	}
}




