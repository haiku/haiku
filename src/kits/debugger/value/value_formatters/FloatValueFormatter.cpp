/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#include "FloatValueFormatter.h"

#include <new>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

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
	BVariant variantValue = value->GetValue();
	switch (variantValue.Type()) {
		case B_FLOAT_TYPE:
		{
			snprintf(buffer, sizeof(buffer), "%f", variantValue.ToFloat());
			break;
		}
		case B_DOUBLE_TYPE:
		{
			snprintf(buffer, sizeof(buffer), "%g", variantValue.ToDouble());
			break;
		}
	}

	_output.SetTo(buffer);

	return B_OK;
}


bool
FloatValueFormatter::SupportsValidation() const
{
	return true;
}


bool
FloatValueFormatter::ValidateFormattedValue(const BString& input,
	type_code type) const
{
	::Value* value = NULL;
	return _PerformValidation(input, type, value, false) == B_OK;
}


status_t
FloatValueFormatter::GetValueFromFormattedInput(const BString& input,
	type_code type, Value*& _output) const
{
	return _PerformValidation(input, type, _output, true);
}


status_t
FloatValueFormatter::_PerformValidation(const BString& input, type_code type,
	::Value*& _output, bool wantsValue) const
{
	const char* text = input.String();
	char *parseEnd = NULL;
	double parsedValue = strtod(text, &parseEnd);
	if (parseEnd - text < input.Length() && !isspace(*parseEnd))
		return B_NO_MEMORY;

	BVariant newValue;
	switch (type) {
		case B_FLOAT_TYPE:
		{
			newValue.SetTo((float)parsedValue);
			break;
		}
		case B_DOUBLE_TYPE:
		{
			newValue.SetTo(parsedValue);
			break;
		}
		default:
			return B_BAD_VALUE;
	}
	if (wantsValue) {
		_output = new(std::nothrow) FloatValue(newValue);
		if (_output == NULL)
			return B_NO_MEMORY;
	}

	return B_OK;
}
