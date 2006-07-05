/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "SwatchValueView.h"

#include <stdio.h>

// constructor
SwatchValueView::SwatchValueView(const char* name,
								 BMessage* message,
								 BHandler* target,
								 rgb_color color,
								 float width,
								 float height)
	: SwatchView(name, message, target, color, width, height)
{
	uint32 flags = Flags();
	flags |= B_NAVIGABLE;
	SetFlags(flags);
}

// destructor
SwatchValueView::~SwatchValueView()
{
}

// MakeFocus
void
SwatchValueView::MakeFocus(bool focused)
{
	BView::MakeFocus(focused);
	if (BView* parent = Parent())
		parent->Invalidate();
}

// Draw
void
SwatchValueView::Draw(BRect updateRect)
{
	BRect b(Bounds());
	if (BView* parent = Parent()) {
		SetLowColor(tint_color(parent->LowColor(), B_DARKEN_1_TINT));
		StrokeRect(b, B_SOLID_LOW);
		b.InsetBy(1.0, 1.0);
	}
	FillRect(b);
}

// MouseDown
void
SwatchValueView::MouseDown(BPoint where)
{
	// forward click
	if (BView* parent = Parent())
		parent->MouseDown(ConvertToParent(where));

	SwatchView::MouseDown(where);
}
