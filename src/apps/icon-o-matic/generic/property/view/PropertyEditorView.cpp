/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "PropertyEditorView.h"

#include <stdio.h>

#include "Property.h"
#include "PropertyItemView.h"

// constructor
PropertyEditorView::PropertyEditorView()
	: BView(BRect(0.0, 0.0, 10.0, 10.0), "property item",
			B_FOLLOW_LEFT | B_FOLLOW_TOP,
			B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE),
	  fParent(NULL),
	  fSelected(false)
{
}

// destructor
PropertyEditorView::~PropertyEditorView()
{
}

// Draw
void
PropertyEditorView::Draw(BRect updateRect)
{
	// just draw background
	FillRect(Bounds(), B_SOLID_LOW);
}

// MouseDown
void
PropertyEditorView::MouseDown(BPoint where)
{
	if (fParent) {
		// forward click
		fParent->MouseDown(ConvertToParent(where));
	}
}

// MouseUp
void
PropertyEditorView::MouseUp(BPoint where)
{
	if (fParent) {
		// forward click
		fParent->MouseUp(ConvertToParent(where));
	}
}

// MouseMoved
void
PropertyEditorView::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	if (fParent) {
		// forward click
		fParent->MouseMoved(ConvertToParent(where), transit, dragMessage);
	}
}

// PreferredHeight
float
PropertyEditorView::PreferredHeight() const
{
	font_height fh;
	GetFontHeight(&fh);

	float height = floorf(4.0 + fh.ascent + fh.descent);

	return height;
}

// SetSelected
void
PropertyEditorView::SetSelected(bool selected)
{
	fSelected = selected;
}

// SetItemView
void
PropertyEditorView::SetItemView(PropertyItemView* parent)
{
	fParent = parent;
	if (fParent) {
		BFont font;
		fParent->GetFont(&font);
		SetFont(&font);
		SetLowColor(fParent->LowColor());
	}
}

// ValueChanged
void
PropertyEditorView::ValueChanged()
{
	if (fParent)
		fParent->UpdateObject();
}

