/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "Int64ValueView.h"

#include <stdio.h>

#include "NummericalTextView.h"

// constructor
Int64ValueView::Int64ValueView(Int64Property* property)
	: TextInputValueView(),
	  fProperty(property)
{
	BRect b = Bounds();
	fTextView = new NummericalTextView(b, "nummerical input", b,
									   B_FOLLOW_LEFT | B_FOLLOW_TOP,
									   B_WILL_DRAW);

	AddChild(fTextView);
	fTextView->SetFloatMode(false);

// TODO: make NummericalTextView support int64?
	if (fProperty)
		fTextView->SetValue((int32)fProperty->Value());
}

// destructor
Int64ValueView::~Int64ValueView()
{
}

// TextView
InputTextView*
Int64ValueView::TextView() const
{
	return fTextView;
}

// ValueChanged
void
Int64ValueView::ValueChanged()
{
	if (fProperty) {
		fProperty->SetValue(fTextView->IntValue());
// TODO: make NummericalTextView support int64?
		fTextView->SetValue((int32)fProperty->Value());
		TextInputValueView::ValueChanged();
	}
}

// AdoptProperty
bool
Int64ValueView::AdoptProperty(Property* property)
{
	Int64Property* p = dynamic_cast<Int64Property*>(property);
	if (p) {
// TODO: make NummericalTextView support int64?
		if (fTextView->IntValue() != (int32)p->Value())
			fTextView->SetValue((int32)p->Value());
		fProperty = p;
		return true;
	}
	return false;
}

// GetProperty
Property*
Int64ValueView::GetProperty() const
{
	return fProperty;
}

