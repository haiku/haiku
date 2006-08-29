/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "IconObject.h"

#include <Message.h>

#include "CommonPropertyIDs.h"
#include "Property.h"
#include "PropertyObject.h"

// constructor
IconObject::IconObject(const char* name)
	: Observable(),
	  Referenceable(),
	  Selectable(),

	  fName(name)
{
}

// copy constructor
IconObject::IconObject(const IconObject& other)
	: Observable(),
	  Referenceable(),
	  Selectable(),

	  fName(other.fName)
{
}

// archive constructor
IconObject::IconObject(BMessage* archive)
	: Observable(),
	  Referenceable(),
	  Selectable(),

	  fName()
{
	// NOTE: uses IconObject version, not overridden
	Unarchive(archive);
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
//	Notify();
}

// #pragma mark -

// Unarchive
status_t
IconObject::Unarchive(const BMessage* archive)
{
	if (!archive)
		return B_BAD_VALUE;

	const char* name;
	status_t ret = archive->FindString("name", &name);

	if (ret == B_OK)
		fName = name;

	return ret;
}

// Archive
status_t
IconObject::Archive(BMessage* into, bool deep) const
{
	if (!into)
		return B_BAD_VALUE;

	return into->AddString("name", fName.String());
}

// MakePropertyObject
PropertyObject*
IconObject::MakePropertyObject() const
{
	PropertyObject* object = new PropertyObject();

	object->AddProperty(new StringProperty(PROPERTY_NAME, fName.String()));

	return object;
}

// SetToPropertyObject
bool
IconObject::SetToPropertyObject(const PropertyObject* object)
{
	AutoNotificationSuspender _(this);

	BString name;
	if (object->GetValue(PROPERTY_NAME, name))
		SetName(name.String());

	return HasPendingNotifications();
}

// SetName
void
IconObject::SetName(const char* name)
{
	if (fName == name)
		return;

	fName = name;
	Notify();
}
