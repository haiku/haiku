/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "ColorValueView.h"

#include <stdio.h>

#include <Message.h>
#include <String.h>
#include <Window.h>

#include "support_ui.h"

#include "PropertyItemView.h"
#include "SwatchValueView.h"

enum {
	MSG_VALUE_CHANGED	= 'vchd',
};

// constructor
ColorValueView::ColorValueView(ColorProperty* property)
	: PropertyEditorView(),
	  fProperty(property)
{
	fSwatchView = new SwatchValueView("swatch property view",
									  NULL, this,/*new BMessage(MSG_SET_COLOR), this,*/
									  fProperty->Value());
	fSwatchView->SetDroppedMessage(new BMessage(MSG_VALUE_CHANGED));
	AddChild(fSwatchView);
}

// destructor
ColorValueView::~ColorValueView()
{
}

// Draw
void
ColorValueView::Draw(BRect updateRect)
{
	BRect b(Bounds());
	if (fSwatchView->IsFocus()) {
		SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		StrokeRect(b);
		b.InsetBy(1.0, 1.0);
		updateRect = updateRect & b;
	}
	FillRect(b, B_SOLID_LOW);
}

// FrameResized
void
ColorValueView::FrameResized(float width, float height)
{
	BRect b(Bounds());
	b.InsetBy(2.0, 2.0);
	b.left = floorf(b.left + (b.Width() / 2.0) - b.Height() / 2.0);
	b.right = b.left + b.Height();
	
	fSwatchView->MoveTo(b.LeftTop());
	fSwatchView->ResizeTo(b.Width(), b.Height());
}

// MakeFocus
void
ColorValueView::MakeFocus(bool focused)
{
	fSwatchView->MakeFocus(focused);
}

// MessageReceived
void
ColorValueView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_PASTE:
			fSwatchView->MessageReceived(message);
			break;
//		case MSG_SET_COLOR:
//			if (BWindow* window = Window())
//				window->PostMessage(message, window);
//			break;
		case MSG_VALUE_CHANGED: {
			rgb_color c;
			if (restore_color_from_message(message, c) >= B_OK
				&& fProperty->SetValue(c)) {
				ValueChanged();
			}
			break;
		}
		default:
			PropertyEditorView::MessageReceived(message);
	}
}

// SetEnabled
void
ColorValueView::SetEnabled(bool enabled)
{
//	if (fEnabled != enabled) {
//		fEnabled = enabled;
//		Invalidate();
//	}
}

// IsFocused
bool
ColorValueView::IsFocused() const
{
	return fSwatchView->IsFocus();
}

// AdoptProperty
bool
ColorValueView::AdoptProperty(Property* property)
{
	ColorProperty* p = dynamic_cast<ColorProperty*>(property);
	if (p) {
		rgb_color ownColor = fProperty->Value();
		rgb_color color = p->Value();
		if (ownColor != color) {
			fSwatchView->SetColor(color);
		}
		fProperty = p;
		return true;
	}
	return false;
}

// GetProperty
Property*
ColorValueView::GetProperty() const
{
	return fProperty;
}



