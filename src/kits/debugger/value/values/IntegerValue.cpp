/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "IntegerValue.h"


IntegerValue::IntegerValue(const BVariant& value)
	:
	fValue(value)
{
}


IntegerValue::~IntegerValue()
{
}


bool
IntegerValue::IsSigned() const
{
	bool isSigned;
	return fValue.IsInteger(&isSigned) && isSigned;
}


bool
IntegerValue::ToString(BString& _string) const
{
	bool isSigned;
	if (!fValue.IsInteger(&isSigned))
		return false;

	BString string;
	if (isSigned)
		string << fValue.ToInt64();
	else
		string << fValue.ToUInt64();

	if (string.Length() == 0)
		return false;

	_string = string;
	return true;
}


bool
IntegerValue::ToVariant(BVariant& _value) const
{
	_value = fValue;
	return true;
}


bool
IntegerValue::operator==(const Value& other) const
{
	const IntegerValue* otherInt = dynamic_cast<const IntegerValue*>(&other);
	return otherInt != NULL ? fValue == otherInt->fValue : false;
}
