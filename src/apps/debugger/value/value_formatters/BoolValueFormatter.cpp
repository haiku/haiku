/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#include "BoolValueFormatter.h"

#include "BoolValue.h"


BoolValueFormatter::BoolValueFormatter()
	:
	ValueFormatter()
{
}


BoolValueFormatter::~BoolValueFormatter()
{
}


status_t
BoolValueFormatter::FormatValue(Value* _value, BString& _output)
{
	BoolValue* value = dynamic_cast<BoolValue*>(_value);
	if (value == NULL)
		return B_BAD_VALUE;

	_output.SetTo(value->GetValue() ? "true" : "false");

	return B_OK;
}
