/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#include "EnumerationValueFormatter.h"

#include "EnumerationValue.h"
#include "Type.h"


EnumerationValueFormatter::EnumerationValueFormatter(Config* config)
	:
	IntegerValueFormatter(config)
{
}


EnumerationValueFormatter::~EnumerationValueFormatter()
{
}


status_t
EnumerationValueFormatter::FormatValue(Value* _value, BString& _output)
{
	Config* config = GetConfig();
	if (config != NULL && config->IntegerFormat() == INTEGER_FORMAT_DEFAULT) {
		EnumerationValue* value = dynamic_cast<EnumerationValue*>(_value);
		if (value == NULL)
			return B_BAD_VALUE;

		if (EnumeratorValue* enumValue
				= value->GetType()->ValueFor(value->GetValue())) {
			_output.SetTo(enumValue->Name());
			return B_OK;
		}
	}

	return IntegerValueFormatter::FormatValue(_value, _output);
}
