/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "Selectable.h"

#include <debugger.h>

#include "Selection.h"

// constructor
Selectable::Selectable()
	: fSelected(false),
	  fSelection(NULL)
{
}

// destructor
Selectable::~Selectable()
{
}

// SetSelected
void
Selectable::SetSelected(bool selected, bool exclusive)
{
	// NOTE: "exclusive" is only useful when selecting,
	// it is ignored when deselecting...
	if (selected == fSelected)
		return;

	if (fSelection) {
		if (selected)
			fSelection->Select(this, !exclusive);
		else
			fSelection->Deselect(this);
	} else {
		debugger("Selectable needs to know Selection\n");
	}
}

// SetSelection
void
Selectable::SetSelection(Selection* selection)
{
	fSelection = selection;
}

// #pragma mark -

// SetSelected
void
Selectable::_SetSelected(bool selected)
{
	// NOTE: for private use by the Selection object!
	if (fSelected != selected) {
		fSelected = selected;
		SelectedChanged();
	}
}
