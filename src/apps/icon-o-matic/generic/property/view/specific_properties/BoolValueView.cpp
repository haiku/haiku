/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "BoolValueView.h"

#include <stdio.h>

#include "ui_defines.h"

// constructor
BoolValueView::BoolValueView(BoolProperty* property)
	: PropertyEditorView(),
	  fProperty(property),
	  fCheckBoxRect(0.0, 0.0, -1.0, -1.0),
	  fEnabled(true)
{
}

// destructor
BoolValueView::~BoolValueView()
{
}

// Draw
void
BoolValueView::Draw(BRect updateRect)
{
	BRect b(Bounds());
	// focus indication
	if (IsFocus()) {
		SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		StrokeRect(b);
		b.InsetBy(1.0, 1.0);
	}
	// background
	FillRect(b, B_SOLID_LOW);

	// checkmark box
	rgb_color crossOutline = kBlack;
	rgb_color crossColor = ui_color(B_KEYBOARD_NAVIGATION_COLOR);

	if (!fEnabled) {
		crossOutline = tint_color(crossOutline, B_LIGHTEN_2_TINT);
		crossColor = tint_color(crossColor, B_LIGHTEN_2_TINT);
	}

	SetHighColor(crossOutline);
	b = fCheckBoxRect;
	StrokeRect(b);

	// checkmark
	if (fProperty && fProperty->Value()) {
		SetHighColor(crossColor);
		b.InsetBy(3.0, 3.0);
		SetPenSize(2.0);
		StrokeLine(b.LeftTop(), b.RightBottom());
		StrokeLine(b.LeftBottom(), b.RightTop());
	}
}

// FrameResized
void
BoolValueView::FrameResized(float width, float height)
{
	float radius = ceilf((height - 6.0) / 2.0);
	float centerX = floorf(Bounds().left + width / 2.0);
	float centerY = floorf(Bounds().top + height / 2.0);
	fCheckBoxRect.Set(centerX - radius, centerY - radius,
					  centerX + radius, centerY + radius);
}

// MakeFocus
void
BoolValueView::MakeFocus(bool focused)
{
	PropertyEditorView::MakeFocus(focused);
	Invalidate();
}

// MouseDown
void
BoolValueView::MouseDown(BPoint where)
{
	MakeFocus(true);	
	if (fCheckBoxRect.Contains(where)) {
		_ToggleValue();
	}
	// NOTE: careful, when this function returns, the object might
	// in fact have been deleted
}

// KeyDown
void
BoolValueView::KeyDown(const char* bytes, int32 numBytes)
{
	bool handled = true;
	if (numBytes > 0) {
		switch (bytes[0]) {
			case B_RETURN:
			case B_SPACE:
			case B_UP_ARROW:
			case B_DOWN_ARROW:
			case B_LEFT_ARROW:
			case B_RIGHT_ARROW:
				_ToggleValue();
				break;
			default:
				handled = false;
				break;
		}
	}
	if (!handled)
		PropertyEditorView::KeyDown(bytes, numBytes);
}

// _ToggleValue
void
BoolValueView::_ToggleValue()
{
	if (!fEnabled)
		return;

	if (fProperty) {
		fProperty->SetValue(!fProperty->Value());
		BRect b(fCheckBoxRect);
		b.InsetBy(1.0, 1.0);
		Invalidate(b);
		ValueChanged();
	}
}

// SetEnabled
void
BoolValueView::SetEnabled(bool enabled)
{
	if (fEnabled != enabled) {
		fEnabled = enabled;
		Invalidate();
	}
}

// AdoptProperty
bool
BoolValueView::AdoptProperty(Property* property)
{
	BoolProperty* p = dynamic_cast<BoolProperty*>(property);
	if (p) {
		BRect b(fCheckBoxRect);
		b.InsetBy(1.0, 1.0);
		Invalidate(b);

		fProperty = p;
		return true;
	}
	return false;
}

// GetProperty
Property*
BoolValueView::GetProperty() const
{
	return fProperty;
}

