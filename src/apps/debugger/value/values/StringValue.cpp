/*
 * Copyright 2010, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "StringValue.h"

#include <stdio.h>


StringValue::StringValue(const char* value)
	:
	fValue(value)
{
}


StringValue::~StringValue()
{
}


bool
StringValue::ToString(BString& _string) const
{
	_string = fValue;
	return true;
}


bool
StringValue::ToVariant(BVariant& _value) const
{
	_value = fValue.String();
	return true;
}


bool
StringValue::operator==(const Value& other) const
{
	const StringValue* otherString = dynamic_cast<const StringValue*>(&other);
	return otherString != NULL ? fValue == otherString->fValue : false;
}
