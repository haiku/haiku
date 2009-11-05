/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "AddressValue.h"

#include <stdio.h>


AddressValue::AddressValue(const BVariant& value)
	:
	IntegerValue(value)
{
}


AddressValue::~AddressValue()
{
}


bool
AddressValue::ToString(BString& _string) const
{
	if (!fValue.IsInteger())
		return false;

	char buffer[32];
	snprintf(buffer, sizeof(buffer), "%#llx", fValue.ToUInt64());

	BString string(buffer);
	if (string.Length() == 0)
		return false;

	_string = string;
	return true;
}
