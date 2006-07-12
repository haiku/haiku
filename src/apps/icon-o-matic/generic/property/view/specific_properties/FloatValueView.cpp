/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "FloatValueView.h"

#include <stdio.h>

#include "NummericalTextView.h"

// constructor
FloatValueView::FloatValueView(FloatProperty* property)
	: TextInputValueView(),
	  fProperty(property)
{
	BRect b = Bounds();
	fTextView = new NummericalTextView(b, "nummerical input", b,
									   B_FOLLOW_LEFT | B_FOLLOW_TOP,
									   B_WILL_DRAW);

	AddChild(fTextView);
	fTextView->SetFloatMode(true);

	if (fProperty)
		fTextView->SetValue(fProperty->Value());
}

// destructor
FloatValueView::~FloatValueView()
{
}

// TextView
InputTextView*
FloatValueView::TextView() const
{
	return fTextView;
}

// ValueChanged
void
FloatValueView::ValueChanged()
{
	if (fProperty) {
		fProperty->SetValue(fTextView->FloatValue());
		fTextView->SetValue(fProperty->Value());
		TextInputValueView::ValueChanged();
	}
}

// AdoptProperty
bool
FloatValueView::AdoptProperty(Property* property)
{
	FloatProperty* p = dynamic_cast<FloatProperty*>(property);
	if (p) {
		if (fTextView->FloatValue() != p->Value())
			fTextView->SetValue(p->Value());
		fProperty = p;
		return true;
	}
	return false;
}

// GetProperty
Property*
FloatValueView::GetProperty() const
{
	return fProperty;
}



