/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "IconObject.h"

// constructor
IconObject::IconObject()
	: Observable(),
	  Referenceable(),
	  Selectable()
{
}

// destructor
IconObject::~IconObject()
{
}

// SelectedChanged
void
IconObject::SelectedChanged()
{
	// simply pass on the event for now
	Notify();
}

// #pragma mark -

// MakePropertyObject
PropertyObject*
IconObject::MakePropertyObject() const
{
	return NULL;
}

// SetToPropertyObject
bool
IconObject::SetToPropertyObject(const PropertyObject* object)
{
	return false;
}

