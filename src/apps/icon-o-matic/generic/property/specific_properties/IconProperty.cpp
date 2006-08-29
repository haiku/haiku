/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "IconProperty.h"

#include <new>
#include <stdio.h>

#include <Message.h>

using std::nothrow;

// constructor
IconProperty::IconProperty(uint32 identifier,
						   const uchar* icon,
						   uint32 width, uint32 height,
						   color_space format,
						   BMessage* message)
	: Property(identifier),
	  fMessage(message),
	  fIcon(icon),
	  fWidth(width),
	  fHeight(height),
	  fFormat(format)
{
}

// archive constructor
IconProperty::IconProperty(const IconProperty& other)
	: Property(other),
	  fMessage(other.fMessage ? new BMessage(*other.fMessage) : NULL),
	  fIcon(other.fIcon),
	  fWidth(other.fWidth),
	  fHeight(other.fHeight),
	  fFormat(other.fFormat)
{
}

// archive constructor
IconProperty::IconProperty(BMessage* archive)
	: Property(archive),
	  fMessage(new BMessage())
{
	if (archive->FindMessage("message", fMessage) < B_OK) {
		delete fMessage;
		fMessage = NULL;
	}
}

// destrucor
IconProperty::~IconProperty()
{
	delete fMessage;
}

// Archive
status_t
IconProperty::Archive(BMessage* into, bool deep) const
{
	status_t status = Property::Archive(into, deep);

	if (status >= B_OK && fMessage)
		status = into->AddMessage("message", fMessage);

	// finish off
	if (status >= B_OK)
		status = into->AddString("class", "IconProperty");

	return status;
}

// Instantiate
BArchivable*
IconProperty::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "IconProperty"))
		return new IconProperty(archive);
	return NULL;
}

// #pragma mark -

// Clone
Property*
IconProperty::Clone() const
{
	return new (nothrow) IconProperty(*this);
}

// SetValue
bool
IconProperty::SetValue(const char* str)
{
	return false;
}

// SetValue
bool
IconProperty::SetValue(const Property* other)
{
	const IconProperty* i = dynamic_cast<const IconProperty*>(other);
	if (i) {
		SetMessage(i->Message());
		return true;
	}
	return false;
}

// GetValue
void
IconProperty::GetValue(BString& string)
{
	string << "dummy";
}

// InterpolateTo
bool
IconProperty::InterpolateTo(const Property* other, float scale)
{
	return false;
}

// #pragma mark -

// SetMessage
void
IconProperty::SetMessage(const BMessage* message)
{
	if (message && fMessage) {
		*fMessage = *message;
	}
}

