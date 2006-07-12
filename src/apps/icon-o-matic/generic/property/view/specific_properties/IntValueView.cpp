/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "IntValueView.h"

#include <stdio.h>

#include "NummericalTextView.h"

// constructor
IntValueView::IntValueView(IntProperty* property)
	: TextInputValueView(),
	  fProperty(property)
{
	BRect b = Bounds();
	fTextView = new NummericalTextView(b, "nummerical input", b,
									   B_FOLLOW_LEFT | B_FOLLOW_TOP,
									   B_WILL_DRAW);

	AddChild(fTextView);
	fTextView->SetFloatMode(false);

	if (fProperty)
		fTextView->SetValue(fProperty->Value());
}

// destructor
IntValueView::~IntValueView()
{
}

// TextView
InputTextView*
IntValueView::TextView() const
{
	return fTextView;
}

// ValueChanged
void
IntValueView::ValueChanged()
{
	if (fProperty) {
		fProperty->SetValue(fTextView->IntValue());
		fTextView->SetValue(fProperty->Value());
		TextInputValueView::ValueChanged();
	}
}

// AdoptProperty
bool
IntValueView::AdoptProperty(Property* property)
{
	IntProperty* p = dynamic_cast<IntProperty*>(property);
	if (p) {
		if (fTextView->IntValue() != p->Value())
			fTextView->SetValue(p->Value());
		fProperty = p;
		return true;
	}
	return false;
}

// GetProperty
Property*
IntValueView::GetProperty() const
{
	return fProperty;
}

