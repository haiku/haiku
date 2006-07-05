/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "TextInputValueView.h"

#include <stdio.h>

#include <Message.h>
#include <String.h>

#include "NummericalTextView.h"
#include "PropertyItemView.h"

enum {
	MSG_VALUE_CHANGED	= 'vchd',
};

// constructor
TextInputValueView::TextInputValueView()
	: PropertyEditorView()
{
}

// destructor
TextInputValueView::~TextInputValueView()
{
}

// AttachedToWindow
void
TextInputValueView::AttachedToWindow()
{
	TextView()->SetMessage(new BMessage(MSG_VALUE_CHANGED));
	TextView()->SetTarget(this);
}

// Draw
void
TextInputValueView::Draw(BRect updateRect)
{
	BRect b(Bounds());
	if (TextView()->IsFocus())
		SetLowColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
	StrokeRect(b, B_SOLID_LOW);
}

// FrameResized
void
TextInputValueView::FrameResized(float width, float height)
{
	BRect b(Bounds());
	b.InsetBy(1.0, 1.0);
	TextView()->MoveTo(b.LeftTop());
	TextView()->ResizeTo(b.Width(), b.Height());
	BRect tr(TextView()->Bounds());
	tr.InsetBy(4.0, 1.0);
	TextView()->SetTextRect(tr);
}

// MakeFocus
void
TextInputValueView::MakeFocus(bool focused)
{
	TextView()->MakeFocus(focused);
}

// MessageReceived
void
TextInputValueView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_VALUE_CHANGED:
			ValueChanged();
			break;
		default:
			PropertyEditorView::MessageReceived(message);
	}
}

// SetEnabled
void
TextInputValueView::SetEnabled(bool enabled)
{
	TextView()->MakeEditable(enabled);

	rgb_color textColor = TextView()->LowColor();
	if (enabled)
		textColor = tint_color(textColor, B_DARKEN_MAX_TINT);
	else
		textColor = tint_color(textColor, B_DISABLED_LABEL_TINT);
	TextView()->SetFontAndColor(NULL, 0, &textColor);
}

// IsFocused
bool
TextInputValueView::IsFocused() const
{
	return TextView()->IsFocus();
}


