/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "PropertyObject.h"

#include <stdio.h>

#include <ByteOrder.h>
#include <Message.h>
#include <OS.h>

#include "Int64Property.h"
#include "Property.h"

#ifndef DEBUG
#	define DEBUG 0
#endif

// constructor
PropertyObject::PropertyObject()
	: fProperties(16)
{
}

// constructor
PropertyObject::PropertyObject(const PropertyObject& other)
	: fProperties(16)
{
	Assign(other);
}

// destructor
PropertyObject::~PropertyObject()
{
	DeleteProperties();
}

// #pragma mark -

// Archive
status_t
PropertyObject::Archive(BMessage* into) const
{
	if (!into)
		return B_BAD_VALUE;

	status_t ret = B_OK;

	int32 count = CountProperties();
	for (int32 i = 0; i < count; i++) {
		Property* p = PropertyAtFast(i);

		unsigned id = B_HOST_TO_BENDIAN_INT32(p->Identifier());
		char idString[5];
		sprintf(idString, "%.4s", (const char*)&id);
		idString[4] = 0;

		BString value;
		p->GetValue(value);
		ret = into->AddString(idString, value.String());
		if (ret < B_OK)
			return ret;
	}
	return ret;
}

// Unarchive
status_t
PropertyObject::Unarchive(const BMessage* archive)
{
	if (!archive)
		return B_BAD_VALUE;

	SuspendNotifications(true);

	int32 count = CountProperties();
	for (int32 i = 0; i < count; i++) {
		Property* p = PropertyAtFast(i);

		unsigned id = B_HOST_TO_BENDIAN_INT32(p->Identifier());
		char idString[5];
		sprintf(idString, "%.4s", (const char*)&id);
		idString[4] = 0;

		const char* value;
		if (archive->FindString(idString, &value) == B_OK)
			if (p->SetValue(value))
				Notify();
	}

	SuspendNotifications(false);

	return B_OK;
}

// #pragma mark -

// AddProperty
bool
PropertyObject::AddProperty(Property* property)
{
	if (!property)
		return false;

#if DEBUG
	if (FindProperty(property->Identifier())) {
		debugger("PropertyObject::AddProperty() -"
				 "property with same ID already here!\n");
		return false;
	}

	// debugging
	if (fProperties.HasItem((void*)property)) {
		debugger("PropertyObject::AddProperty() - property id "
				 "changed unexpectedly\n");
		return false;
	}
#endif

	if (fProperties.AddItem((void*)property)) {
		Notify();
		return true;
	}

	return false;
}

// PropertyAt
Property*
PropertyObject::PropertyAt(int32 index) const
{
	return (Property*)fProperties.ItemAt(index);
}

// PropertyAtFast
Property*
PropertyObject::PropertyAtFast(int32 index) const
{
	return (Property*)fProperties.ItemAtFast(index);
}

// CountProperties
int32
PropertyObject::CountProperties() const
{
	return fProperties.CountItems();
}

// #pragma mark -

// FindProperty
Property*
PropertyObject::FindProperty(uint32 propertyID) const
{
	int32 count = fProperties.CountItems();
	for (int32 i = 0; i < count; i++) {
		Property* p = (Property*)fProperties.ItemAtFast(i);
		if (p->Identifier() == propertyID)
			return p;
	}
	return NULL;
}

//HasProperty
bool
PropertyObject::HasProperty(Property* property) const
{
	return fProperties.HasItem((void*)property);
}

// ContainsSameProperties
bool
PropertyObject::ContainsSameProperties(const PropertyObject& other) const
{
	bool equal = false;
	int32 count = CountProperties();
	if (count == other.CountProperties()) {
		equal = true;
		for (int32 i = 0; i < count; i++) {
			Property* ownProperty = PropertyAtFast(i);
			Property* otherProperty = other.PropertyAtFast(i);
			if (ownProperty->Identifier() != otherProperty->Identifier()) {
				equal = false;
				break;
			}
		}
	}
	return equal;
}

// Assign
status_t
PropertyObject::Assign(const PropertyObject& other)
{
	DeleteProperties();

	int32 count = other.fProperties.CountItems();
	for (int32 i = 0; i < count; i++) {
		Property* p = (Property*)other.fProperties.ItemAtFast(i);
		Property* clone = p->Clone();
		if (!AddProperty(clone)) {
			delete clone;
			fprintf(stderr, "PropertyObject::Assign() - no memory for "
							"cloning properties!\n");
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}

// DeleteProperties
void
PropertyObject::DeleteProperties()
{
	int32 count = fProperties.CountItems();
	for (int32 i = 0; i < count; i++)
		delete (Property*)fProperties.ItemAtFast(i);
	fProperties.MakeEmpty();
	Notify();
}

// DeleteProperty
bool
PropertyObject::DeleteProperty(uint32 propertyID)
{
	int32 count = fProperties.CountItems();
	for (int32 i = 0; i < count; i++) {
		Property* p = (Property*)fProperties.ItemAtFast(i);
		if (p->Identifier() == propertyID) {
			if (fProperties.RemoveItem(i)) {
				Notify();
				delete p;
				return true;
			}
			break;
		}
	}
	return false;
}

// ValueChanged
void
PropertyObject::ValueChanged(uint32 propertyID)
{
	Notify();
}

// #pragma mark -

// SetValue
bool
PropertyObject::SetValue(uint32 propertyID, const char* value)
{
	if (Property* p = FindProperty(propertyID)) {
		if (p->SetValue(value)) {
			ValueChanged(propertyID);
			return true;
		}
	}
	return false;
}

// GetValue
bool
PropertyObject::GetValue(uint32 propertyID, BString& value) const
{
	if (Property* p = FindProperty(propertyID)) {
		p->GetValue(value);
		return true;
	}
	return false;
}

// #pragma mark - int32

// SetValue
bool
PropertyObject::SetValue(uint32 propertyID, int32 value)
{
	IntProperty* p = dynamic_cast<IntProperty*>(FindProperty(propertyID));
	if (p && p->SetValue(value)) {
		ValueChanged(propertyID);
		return true;
	}
	return false;
}

// Value
int32
PropertyObject::Value(uint32 propertyID, int32 defaultValue) const
{
	if (IntProperty* p = dynamic_cast<IntProperty*>(FindProperty(propertyID)))
		return p->Value();
	return defaultValue;
}

// #pragma mark - int64

// SetValue
bool
PropertyObject::SetValue(uint32 propertyID, int64 value)
{
	Int64Property* p = dynamic_cast<Int64Property*>(FindProperty(propertyID));
	if (p && p->SetValue(value)) {
		ValueChanged(propertyID);
		return true;
	}
	return false;
}

// Value
int64
PropertyObject::Value(uint32 propertyID, int64 defaultValue) const
{
	if (Int64Property* p = dynamic_cast<Int64Property*>(FindProperty(propertyID)))
		return p->Value();
	return defaultValue;
}

// #pragma mark - float

// SetValue
bool
PropertyObject::SetValue(uint32 propertyID, float value)
{
	FloatProperty* p = dynamic_cast<FloatProperty*>(FindProperty(propertyID));
	if (p && p->SetValue(value)) {
		ValueChanged(propertyID);
		return true;
	}
	return false;
}

// Value
float
PropertyObject::Value(uint32 propertyID, float defaultValue) const
{
	if (FloatProperty* p = dynamic_cast<FloatProperty*>(FindProperty(propertyID)))
		return p->Value();
	return defaultValue;
}

// #pragma mark - bool

// SetValue
bool
PropertyObject::SetValue(uint32 propertyID, bool value)
{
	BoolProperty* p = dynamic_cast<BoolProperty*>(FindProperty(propertyID));
	if (p && p->SetValue(value)) {
		ValueChanged(propertyID);
		return true;
	}
	return false;
}

// Value
bool
PropertyObject::Value(uint32 propertyID, bool defaultValue) const
{
	if (BoolProperty* p = dynamic_cast<BoolProperty*>(FindProperty(propertyID)))
		return p->Value();
	return defaultValue;
}


