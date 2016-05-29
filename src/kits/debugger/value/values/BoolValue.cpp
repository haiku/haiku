/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "BoolValue.h"


BoolValue::BoolValue(bool value)
	:
	fValue(value)
{
}


BoolValue::~BoolValue()
{
}


bool
BoolValue::ToString(BString& _string) const
{
	BString string = fValue ? "true" : "false";
	if (string.Length() == 0)
		return false;

	_string = string;
	return true;
}


bool
BoolValue::ToVariant(BVariant& _value) const
{
	_value = fValue;
	return true;
}


bool
BoolValue::operator==(const Value& other) const
{
	const BoolValue* otherBool = dynamic_cast<const BoolValue*>(&other);
	return otherBool != NULL ? fValue == otherBool->fValue : false;
}
