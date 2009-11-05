/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "AddressValueHandler.h"

#include "AddressValue.h"


float
AddressValueHandler::SupportsValue(Value* value)
{
	return dynamic_cast<AddressValue*>(value) ? 0.8f : 0;
}


integer_format
AddressValueHandler::DefaultIntegerFormat(IntegerValue* value)
{
	return INTEGER_FORMAT_HEX_DEFAULT;
}
