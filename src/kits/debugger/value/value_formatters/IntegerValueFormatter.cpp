/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#include "IntegerValueFormatter.h"

#include <new>

#include <ctype.h>

#include "IntegerFormatter.h"
#include "IntegerValue.h"


// #pragma mark - IntegerValueFormatter


IntegerValueFormatter::IntegerValueFormatter(Config* config)
	:
	ValueFormatter(),
	fConfig(config)
{
	if (fConfig != NULL)
		fConfig->AcquireReference();
}


IntegerValueFormatter::~IntegerValueFormatter()
{
	if (fConfig != NULL)
		fConfig->ReleaseReference();
}


Settings*
IntegerValueFormatter::GetSettings() const
{
	return fConfig != NULL ? fConfig->GetSettings() : NULL;
}


status_t
IntegerValueFormatter::FormatValue(Value* _value, BString& _output)
{
	IntegerValue* value = dynamic_cast<IntegerValue*>(_value);
	if (value == NULL)
		return B_BAD_VALUE;

	// format the value
	integer_format format = fConfig != NULL
		? fConfig->IntegerFormat() : INTEGER_FORMAT_DEFAULT;
	char buffer[32];
	if (!IntegerFormatter::FormatValue(value->GetValue(), format,  buffer,
			sizeof(buffer))) {
		return B_BAD_VALUE;
	}

	_output.SetTo(buffer);

	return B_OK;
}


bool
IntegerValueFormatter::SupportsValidation() const
{
	return true;
}


bool
IntegerValueFormatter::ValidateFormattedValue(const BString& input,
	type_code type) const
{
	::Value* value = NULL;
	return _PerformValidation(input, type, value, false) == B_OK;
}


status_t
IntegerValueFormatter::GetValueFromFormattedInput(const BString& input,
	type_code type, Value*& _output) const
{
	return _PerformValidation(input, type, _output, true);
}


status_t
IntegerValueFormatter::_PerformValidation(const BString& input, type_code type,
	::Value*& _output, bool wantsValue) const
{
	integer_format format;
	if (fConfig != NULL)
		format = fConfig->IntegerFormat();
	else {
		bool isSigned;
		if (BVariant::TypeIsInteger(type, &isSigned)) {
			format = isSigned ? INTEGER_FORMAT_SIGNED
				: INTEGER_FORMAT_UNSIGNED;
		} else
			return B_BAD_VALUE;
	}

	status_t error = B_OK;
	if (format == INTEGER_FORMAT_UNSIGNED
		|| format >= INTEGER_FORMAT_HEX_DEFAULT) {
		error = _ValidateUnsigned(input, type, _output, format, wantsValue);
	} else
		error = _ValidateSigned(input, type, _output, wantsValue);

	return error;
}


status_t
IntegerValueFormatter::_ValidateSigned(const BString& input, type_code type,
	::Value*& _output, bool wantsValue) const
{
	const char* text = input.String();
	char *parseEnd = NULL;
	intmax_t parsedValue = strtoimax(text, &parseEnd, 10);
	if (parseEnd - text < input.Length() && !isspace(*parseEnd))
		return B_NO_MEMORY;

	BVariant newValue;
	switch (type) {
		case B_INT8_TYPE:
		{
			if (parsedValue < INT8_MIN || parsedValue > INT8_MAX)
				return B_BAD_VALUE;

			newValue.SetTo((int8)parsedValue);
			break;
		}
		case B_INT16_TYPE:
		{
			if (parsedValue < INT16_MIN || parsedValue > INT16_MAX)
				return B_BAD_VALUE;

			newValue.SetTo((int16)parsedValue);
			break;
		}
		case B_INT32_TYPE:
		{
			if (parsedValue < INT32_MIN || parsedValue > INT32_MAX)
				return B_BAD_VALUE;

			newValue.SetTo((int32)parsedValue);
			break;
		}
		case B_INT64_TYPE:
		{
			newValue.SetTo((int64)parsedValue);
			break;
		}
		default:
			return B_BAD_VALUE;
	}

	if (wantsValue) {
		_output = new(std::nothrow) IntegerValue(newValue);
		if (_output == NULL)
			return B_NO_MEMORY;
	}

	return B_OK;
}


status_t
IntegerValueFormatter::_ValidateUnsigned(const BString& input, type_code type,
	::Value*& _output, integer_format format, bool wantsValue) const
{
	const char* text = input.String();
	int32 base = format == INTEGER_FORMAT_UNSIGNED ? 10 : 16;

	char *parseEnd = NULL;
	uintmax_t parsedValue = strtoumax(text, &parseEnd, base);
	if (parseEnd - text < input.Length() && !isspace(*parseEnd))
		return B_BAD_VALUE;

	BVariant newValue;
	switch (type) {
		case B_UINT8_TYPE:
		{
			if (parsedValue > UINT8_MAX)
				return B_BAD_VALUE;

			newValue.SetTo((uint8)parsedValue);
			break;
		}
		case B_UINT16_TYPE:
		{
			if (parsedValue > UINT16_MAX)
				return B_BAD_VALUE;

			newValue.SetTo((uint16)parsedValue);
			break;
		}
		case B_UINT32_TYPE:
		{
			if (parsedValue > UINT32_MAX)
				return B_BAD_VALUE;

			newValue.SetTo((uint32)parsedValue);
			break;
		}
		case B_UINT64_TYPE:
		{
			newValue.SetTo((uint64)parsedValue);
			break;
		}
		default:
			return B_BAD_VALUE;
	}

	if (wantsValue) {
		_output = new(std::nothrow) IntegerValue(newValue);
		if (_output == NULL)
			return B_NO_MEMORY;
	}

	return B_OK;
}



// #pragma mark - Config


IntegerValueFormatter::Config::~Config()
{
}
