/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#include "FloatValueFormatter.h"

#include <stdio.h>

#include "FloatValue.h"


FloatValueFormatter::FloatValueFormatter()
	:
	ValueFormatter()
{
}


FloatValueFormatter::~FloatValueFormatter()
{
}


status_t
FloatValueFormatter::FormatValue(Value* _value, BString& _output)
{
	FloatValue* value = dynamic_cast<FloatValue*>(_value);
	if (value == NULL)
		return B_BAD_VALUE;

	char buffer[64];
	snprintf(buffer, sizeof(buffer), "%g", value->GetValue());

	_output.SetTo(buffer);

	return B_OK;
}
