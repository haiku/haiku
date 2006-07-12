/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "StringValueView.h"

#include <stdio.h>
#include <string.h>

#include "StringTextView.h"

// constructor
StringValueView::StringValueView(StringProperty* property)
	: TextInputValueView(),
	  fProperty(property)
{
	BRect b = Bounds();
	fTextView = new StringTextView(b, "string input", b,
								   B_FOLLOW_LEFT | B_FOLLOW_TOP,
								   B_WILL_DRAW);
	AddChild(fTextView);

	if (fProperty)
		fTextView->SetValue(fProperty->Value());
}

// destructor
StringValueView::~StringValueView()
{
}

// TextView
InputTextView*
StringValueView::TextView() const
{
	return fTextView;
}

// ValueChanged
void
StringValueView::ValueChanged()
{
	if (fProperty) {
		fProperty->SetValue(fTextView->Value());
		fTextView->SetValue(fProperty->Value());
		TextInputValueView::ValueChanged();
	}
}

// AdoptProperty
bool
StringValueView::AdoptProperty(Property* property)
{
	StringProperty* p = dynamic_cast<StringProperty*>(property);
	if (p) {
		if (!fProperty || strcmp(p->Value(), fTextView->Text()) != 0) {
			fTextView->SetValue(p->Value());
		}
		fProperty = p;
		return true;
	}
	return false;
}

// GetProperty
Property*
StringValueView::GetProperty() const
{
	return fProperty;
}
