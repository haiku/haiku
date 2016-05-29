/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "ValueFormatter.h"


ValueFormatter::~ValueFormatter()
{
}


bool
ValueFormatter::SupportsValidation() const
{
	return false;
}


bool
ValueFormatter::ValidateFormattedValue(const BString& input, type_code type)
	const
{
	return false;
}


status_t
ValueFormatter::GetValueFromFormattedInput(const BString& input,
	type_code type, Value*& _output) const
{
	return B_NOT_SUPPORTED;
}
