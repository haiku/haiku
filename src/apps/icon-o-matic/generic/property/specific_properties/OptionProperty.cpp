/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "OptionProperty.h"

#include <new>
#include <stdio.h>
#include <stdlib.h>

#include <ByteOrder.h>
#include <Message.h>

using std::nothrow;

// constructor
OptionProperty::OptionProperty(uint32 identifier)
	: Property(identifier),
	  fOptions(4),
	  fCurrentOptionID(-1)
{
}

// copy constructor
OptionProperty::OptionProperty(const OptionProperty& other)
	: Property(other),
	  fOptions(4),
	  fCurrentOptionID(other.fCurrentOptionID)
{
	// clone the actual options
	int32 count = other.fOptions.CountItems();
	for (int32 i = 0; i < count; i++) {
		option* o = (option*)(other.fOptions.ItemAtFast(i));
		option* clone = new (nothrow) option;
		if (!clone || !fOptions.AddItem(clone)) {
			delete clone;
			break;
		}
		clone->id = o->id;
		clone->name = o->name;
	}
}

// archive constructor
OptionProperty::OptionProperty(BMessage* archive)
	: Property(archive),
	  fOptions(4),
	  fCurrentOptionID(-1)
{
	if (!archive)
		return;

	if (archive->FindInt32("option", &fCurrentOptionID) < B_OK)
		fCurrentOptionID = -1;
}

// destrucor
OptionProperty::~OptionProperty()
{
	int32 count = fOptions.CountItems();
	for (int32 i = 0; i < count; i++)
		delete (option*)fOptions.ItemAtFast(i);
}

// #pragma mark -

// Archive
status_t
OptionProperty::Archive(BMessage* into, bool deep) const
{
	status_t status = Property::Archive(into, deep);

	if (status >= B_OK)
		status = into->AddInt32("option", fCurrentOptionID);

	// finish off
	if (status >= B_OK)
		status = into->AddString("class", "OptionProperty");

	return status;
}

// Instantiate
BArchivable*
OptionProperty::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "OptionProperty"))
		return new (nothrow) OptionProperty(archive);
	return NULL;
}

// #pragma mark -

// Clone
Property*
OptionProperty::Clone() const
{
	return new (nothrow) OptionProperty(*this);
}

// Type
type_code
OptionProperty::Type() const
{
	// NOTE: not a Be defined type (those are upper case)
	return 'optn';
}

// SetValue
bool
OptionProperty::SetValue(const char* value)
{
	// try to find option by name
	int32 count = fOptions.CountItems();
	for (int32 i = 0; i < count; i++) {
		option* o = (option*)fOptions.ItemAtFast(i);
		if (strcmp(o->name.String(), value) == 0) {
			return SetCurrentOptionID(o->id);
		}
	}

	// try to find option by id
	int32 id = atoi(value);
	if (id < 0)
		return false;

	for (int32 i = 0; i < count; i++) {
		option* o = (option*)fOptions.ItemAtFast(i);
		if (o->id == id) {
			return SetCurrentOptionID(o->id);
		}
	}
	return false;
}

// SetValue
bool
OptionProperty::SetValue(const Property* other)
{
	const OptionProperty* optOther = dynamic_cast<const OptionProperty*>(other);
	if (optOther) {
		return SetCurrentOptionID(optOther->CurrentOptionID());
	}
	return false;
}

// GetValue
void
OptionProperty::GetValue(BString& string)
{
	if (!GetCurrentOption(&string))
		string << fCurrentOptionID;
}

// MakeAnimatable
bool
OptionProperty::MakeAnimatable(bool animatable)
{
	return false;
}

// #pragma mark -

// AddOption
void
OptionProperty::AddOption(int32 id, const char* name)
{
	if (!name)
		return;

	if (option* o = new (nothrow) option) {
		o->id = id;
		o->name = name;
		fOptions.AddItem((void*)o);
	}
}

// CurrentOptionID
int32
OptionProperty::CurrentOptionID() const
{
	return fCurrentOptionID;
}

// SetCurrentOptionID
bool
OptionProperty::SetCurrentOptionID(int32 id)
{
	if (fCurrentOptionID != id) {
		fCurrentOptionID = id;
		return true;
	}
	return false;
}

// GetOption
bool
OptionProperty::GetOption(int32 index, BString* string, int32* id) const
{
	if (option* o = (option*)fOptions.ItemAt(index)) {
		*id = o->id;
		*string = o->name;
		return true;
	} else {
		*id = -1;
		*string = "";
		return false;
	}
}

// GetCurrentOption
bool
OptionProperty::GetCurrentOption(BString* string) const
{
	for (int32 i = 0; option* o = (option*)fOptions.ItemAt(i); i++) {
		if (o->id == fCurrentOptionID) {
			*string = o->name;
			return true;
		}
	}
uint32 current = B_HOST_TO_BENDIAN_INT32(fCurrentOptionID);
printf("OptionProperty::GetCurrentOption() - "
	"did not find option %.4s!!\n", (char*)&current);
	return false;
}

// SetOptionAtOffset
bool
OptionProperty::SetOptionAtOffset(int32 indexOffset)
{
	// NOTE: used by the Property editor GUI
	if (fOptions.CountItems() > 1) {
		int32 index = -1;
		for (int32 i = 0; option* o = (option*)fOptions.ItemAt(i); i++) {
			if (o->id == fCurrentOptionID) {
				index = i;
			}
		}
		if (index >= 0) {
			// offset index
			index += indexOffset;
			// keep index in range by wrapping arround
			if (index >= fOptions.CountItems())
				index = 0;
			if (index < 0)
				index = fOptions.CountItems() - 1;
			if (option* o = (option*)fOptions.ItemAt(index)) {
				SetCurrentOptionID(o->id);
				return true;
			}
		}
	}
	return false;
}



