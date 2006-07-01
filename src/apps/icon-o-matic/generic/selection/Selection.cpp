/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "Selection.h"

#include <stdio.h>

#include <debugger.h>

#include "Selectable.h"

#define DEBUG 1

// constructor
Selection::Selection()
	: fSelected(20)
{
}

// destructor
Selection::~Selection()
{
}

// Select
bool
Selection::Select(Selectable* object, bool extend)
{
	AutoNotificationSuspender _(this);

	if (!extend)
		_DeselectAllExcept(object);

	bool success = false;

	if (!object->IsSelected()) {

		#if DEBUG
		if (fSelected.HasItem((void*)object))
			debugger("Selection::Select() - "
					 "unselected object in list!");
		#endif

		if (fSelected.AddItem((void*)object)) {
			object->_SetSelected(true);
			success = true;

			Notify();
		} else {
			fprintf(stderr, "Selection::Select() - out of memory\n");
		}
	} else {

		#if DEBUG
		if (!fSelected.HasItem((void*)object))
			debugger("Selection::Select() - "
					 "already selected object not in list!");
		#endif

		success = true;
		// object already in list
	}

	return success;
}

// Deselect
void
Selection::Deselect(Selectable* object)
{
	if (object->IsSelected()) {
		if (!fSelected.RemoveItem((void*)object))
			debugger("Selection::Deselect() - "
					 "selected object not within list!");
		object->_SetSelected(false);

		Notify();
	}
}

// DeselectAll
void
Selection::DeselectAll()
{
	_DeselectAllExcept(NULL);
}

// #pragma mark -

// SelectableAt
Selectable*
Selection::SelectableAt(int32 index) const
{
	return (Selectable*)fSelected.ItemAt(index);
}

// SelectableAtFast
Selectable*
Selection::SelectableAtFast(int32 index) const
{
	return (Selectable*)fSelected.ItemAtFast(index);
}

// CountSelected
int32
Selection::CountSelected() const
{
	return fSelected.CountItems();
}

// #pragma mark -

// _DeselectAllExcept
void
Selection::_DeselectAllExcept(Selectable* except)
{
	bool notify = false;
	bool containedExcept = false;

	int32 count = fSelected.CountItems();
	for (int32 i = 0; i < count; i++) {
		Selectable* object = (Selectable*)fSelected.ItemAtFast(i);
		if (object != except) {
			object->_SetSelected(false);
			notify = true;
		} else {
			containedExcept = true;
		}
	}

	fSelected.MakeEmpty();

	// if the "except" object was previously
	// in the selection, add it again after
	// making the selection list empty
	if (containedExcept)
		fSelected.AddItem(except);

	if (notify)
		Notify();
}

