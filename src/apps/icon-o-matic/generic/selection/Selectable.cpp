/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "Selectable.h"

// constructor
Selectable::Selectable()
	: fSelected(false)
{
}

// destructor
Selectable::~Selectable()
{
}

// SetSelected
void
Selectable::SetSelected(bool selected)
{
	if (fSelected != selected) {
		fSelected = selected;
		SelectedChanged();
	}
}
