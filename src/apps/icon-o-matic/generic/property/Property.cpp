/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "Property.h"

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Message.h>

#include "support.h"

using std::nowthrow;

// constructor
Property::Property(uint32 identifier)
	: fIdentifier(identifier),
	  fEditable(true)
{
}

// constructor
Property::Property(const Property& other)
	: fIdentifier(other.fIdentifier),
	  fEditable(other.fEditable)
{
}

// constructor
Property::Property(BMessage* archive)
	: fIdentifier(0),
	  fEditable(true)
{
	if (!archive)
		return;

	if (archive->FindInt32("id", (int32*)&fIdentifier) < B_OK)
		fIdentifier = 0;
	if (archive->FindBool("editable", &fEditable) < B_OK)
		fEditable = true;
}

// destructor
Property::~Property()
{
}

// InterpolateTo
bool
Property::InterpolateTo(const Property* other, float scale)
{
	// some properties don't support this
	return false;
}

// SetEditable
void
Property::SetEditable(bool editable)
{
	fEditable = editable;
}

// #pragma mark -

// constructor
IntProperty::IntProperty(uint32 identifier, int32 value,
						 int32 min, int32 max)
	: Property(identifier),
	  fValue(value),
	  fMin(min),
	  fMax(max)
{
}

// constructor
IntProperty::IntProperty(const IntProperty& other)
	: Property(other),
	  fValue(other.fValue),
	  fMin(other.fMin),
	  fMax(other.fMax)
{
}

// destructor
IntProperty::~IntProperty()
{
}

// Clone
Property*
IntProperty::Clone() const
{
	return new IntProperty(*this);
}

// SetValue
bool
IntProperty::SetValue(const char* value)
{
	return SetValue(atoi(value));
}

// SetValue
bool
IntProperty::SetValue(const Property* other)
{
	const IntProperty* i = dynamic_cast<const IntProperty*>(other);
	if (i) {
		return SetValue(i->Value());
	}
	return false;
}

// GetValue
void
IntProperty::GetValue(BString& string)
{
	string << fValue;
}

// InterpolateTo
bool
IntProperty::InterpolateTo(const Property* other, float scale)
{
	const IntProperty* i = dynamic_cast<const IntProperty*>(other);
	if (i) {
		return SetValue(fValue + (int32)((float)(i->Value()
												 - fValue) * scale + 0.5));
	}
	return false;
}

// SetValue
bool
IntProperty::SetValue(int32 value)
{
	// truncate
	if (value < fMin)
		value = fMin;
	if (value > fMax)
		value = fMax;

	if (value != fValue) {
		fValue = value;
		return true;
	}
	return false;
}

// #pragma mark -

// constructor
FloatProperty::FloatProperty(uint32 identifier, float value,
							 float min, float max)
	: Property(identifier),
	  fValue(value),
	  fMin(min),
	  fMax(max)
{
}

// constructor
FloatProperty::FloatProperty(const FloatProperty& other)
	: Property(other),
	  fValue(other.fValue),
	  fMin(other.fMin),
	  fMax(other.fMax)
{
}

// destructor
FloatProperty::~FloatProperty()
{
}

// Clone
Property*
FloatProperty::Clone() const
{
	return new FloatProperty(*this);
}

// SetValue
bool
FloatProperty::SetValue(const char* value)
{
	return SetValue(atof(value));
}

// SetValue
bool
FloatProperty::SetValue(const Property* other)
{
	const FloatProperty* f = dynamic_cast<const FloatProperty*>(other);
	if (f) {
		return SetValue(f->Value());
	}
	return false;
}

// GetValue
void
FloatProperty::GetValue(BString& string)
{
	append_float(string, fValue, 4);
}

// InterpolateTo
bool
FloatProperty::InterpolateTo(const Property* other, float scale)
{
	const FloatProperty* f = dynamic_cast<const FloatProperty*>(other);
	if (f) {
		return SetValue(fValue + (f->Value() - fValue) * scale);
	}
	return false;
}

// SetValue
bool
FloatProperty::SetValue(float value)
{
	// truncate
	if (value < fMin)
		value = fMin;
	if (value > fMax)
		value = fMax;

	if (value != fValue) {
		fValue = value;
		return true;
	}
	return false;
}

// #pragma mark -

// constructor
UInt8Property::UInt8Property(uint32 identifier, uint8 value)
	: Property(identifier),
	  fValue(value)
{
}

// constructor
UInt8Property::UInt8Property(const UInt8Property& other)
	: Property(other),
	  fValue(other.fValue)
{
}

// destructor
UInt8Property::~UInt8Property()
{
}

// Clone
Property*
UInt8Property::Clone() const
{
	return new UInt8Property(*this);
}

// SetValue
bool
UInt8Property::SetValue(const char* value)
{
	return SetValue((uint8)max_c(0, min_c(255, atoi(value))));
}

// SetValue
bool
UInt8Property::SetValue(const Property* other)
{
	const UInt8Property* u = dynamic_cast<const UInt8Property*>(other);
	if (u) {
		return SetValue(u->Value());
	}
	return false;
}

// GetValue
void
UInt8Property::GetValue(BString& string)
{
	string << fValue;
}

// InterpolateTo
bool
UInt8Property::InterpolateTo(const Property* other, float scale)
{
	const UInt8Property* u = dynamic_cast<const UInt8Property*>(other);
	if (u) {
		return SetValue(fValue + (uint8)((float)(u->Value()
												 - fValue) * scale + 0.5));
	}
	return false;
}

// SetValue
bool
UInt8Property::SetValue(uint8 value)
{
	if (value != fValue) {
		fValue = value;
		return true;
	}
	return false;
}

// #pragma mark -

// constructor
BoolProperty::BoolProperty(uint32 identifier, bool value)
	: Property(identifier),
	  fValue(value)
{
}

// constructor
BoolProperty::BoolProperty(const BoolProperty& other)
	: Property(other),
	  fValue(other.fValue)
{
}

// destructor
BoolProperty::~BoolProperty()
{
}

// Clone
Property*
BoolProperty::Clone() const
{
	return new BoolProperty(*this);
}

// SetValue
bool
BoolProperty::SetValue(const char* value)
{
	bool v;
	if (strcasecmp(value, "true") == 0)
		v = true;
	else if (strcasecmp(value, "on") == 0)
		v = true;
	else
		v = (bool)atoi(value);

	return SetValue(v);
}

// SetValue
bool
BoolProperty::SetValue(const Property* other)
{
	const BoolProperty* b = dynamic_cast<const BoolProperty*>(other);
	if (b) {
		return SetValue(b->Value());
	}
	return false;
}

// GetValue
void
BoolProperty::GetValue(BString& string)
{
	if (fValue)
		string << "on";
	else
		string << "off";
}

// InterpolateTo
bool
BoolProperty::InterpolateTo(const Property* other, float scale)
{
	const BoolProperty* b = dynamic_cast<const BoolProperty*>(other);
	if (b) {
		if (scale >= 0.5)
			return SetValue(b->Value());
	}
	return false;
}

// SetValue
bool
BoolProperty::SetValue(bool value)
{
	if (value != fValue) {
		fValue = value;
		return true;
	}
	return false;
}

// #pragma mark -

// constructor
StringProperty::StringProperty(uint32 identifier, const char* value)
	: Property(identifier),
	  fValue(value)
{
}

// constructor
StringProperty::StringProperty(const StringProperty& other)
	: Property(other),
	  fValue(other.fValue)
{
}

// destructor
StringProperty::~StringProperty()
{
}

// Clone
Property*
StringProperty::Clone() const
{
	return new StringProperty(*this);
}

// SetValue
bool
StringProperty::SetValue(const char* value)
{
	BString t(value);
	if (fValue != t) {
		fValue = t;
		return true;
	}
	return false;
}

// SetValue
bool
StringProperty::SetValue(const Property* other)
{
	const StringProperty* s = dynamic_cast<const StringProperty*>(other);
	if (s) {
		return SetValue(s->Value());
	}
	return false;
}

// GetValue
void
StringProperty::GetValue(BString& string)
{
	string << fValue;
}

