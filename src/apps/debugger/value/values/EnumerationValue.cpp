/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "EnumerationValue.h"

#include "Type.h"


EnumerationValue::EnumerationValue(EnumerationType* type, const BVariant& value)
	:
	IntegerValue(value),
	fType(type)
{
	fType->AcquireReference();
}


EnumerationValue::~EnumerationValue()
{
	fType->ReleaseReference();
}


bool
EnumerationValue::ToString(BString& _string) const
{
	if (!fValue.IsInteger())
		return false;

	EnumeratorValue* enumValue = fType->ValueFor(fValue);
	if (enumValue == NULL)
		return IntegerValue::ToString(_string);

	BString string(enumValue->Name());
	if (string.Length() == 0)
		return false;

	_string = string;
	return true;
}
