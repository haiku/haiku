/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "IntegerFormatter.h"

#include <stdio.h>
#include <string.h>

#include <TypeConstants.h>


static integer_format
GetFormatForTypeAndFormat(type_code type, integer_format format,
	char* _formatString, int formatSize)
{
	integer_format result = format;
	_formatString[0] = '%';
	++_formatString;
	formatSize -= 1;

	switch (type) {
		case B_INT8_TYPE:
			switch (format) {
				case INTEGER_FORMAT_HEX_DEFAULT:
					result = INTEGER_FORMAT_HEX_8;
					break;
				case INTEGER_FORMAT_SIGNED:
					strlcpy(_formatString, B_PRId8, formatSize);
					break;
				case INTEGER_FORMAT_UNSIGNED:
					strlcpy(_formatString, B_PRIu8, formatSize);
					break;
				default:
					break;
			}
			break;
		case B_INT16_TYPE:
			switch (format) {
				case INTEGER_FORMAT_HEX_DEFAULT:
					result = INTEGER_FORMAT_HEX_16;
					break;
				case INTEGER_FORMAT_SIGNED:
					strlcpy(_formatString, B_PRId16, formatSize);
					break;
				case INTEGER_FORMAT_UNSIGNED:
					strlcpy(_formatString, B_PRIu16, formatSize);
					break;
				default:
					break;
			}
			break;
		case B_INT32_TYPE:
			switch (format) {
				case INTEGER_FORMAT_HEX_DEFAULT:
					result = INTEGER_FORMAT_HEX_32;
					break;
				case INTEGER_FORMAT_SIGNED:
					strlcpy(_formatString, B_PRId32, formatSize);
					break;
				case INTEGER_FORMAT_UNSIGNED:
					strlcpy(_formatString, B_PRIu32, formatSize);
					break;
				default:
					break;
			}
			break;
		case B_INT64_TYPE:
		default:
			switch (format) {
				case INTEGER_FORMAT_HEX_DEFAULT:
					result = INTEGER_FORMAT_HEX_64;
					break;
				case INTEGER_FORMAT_SIGNED:
					strlcpy(_formatString, B_PRId64, formatSize);
					break;
				case INTEGER_FORMAT_UNSIGNED:
					strlcpy(_formatString, B_PRIu64, formatSize);
					break;
				default:
					break;
			}
			break;
	}

	return result;
}


/*static*/ bool
IntegerFormatter::FormatValue(const BVariant& value, integer_format format,
	char* buffer, size_t bufferSize)
{
	bool isSigned;
	if (!value.IsInteger(&isSigned))
		return false;

	char formatString[10];

	if (format == INTEGER_FORMAT_DEFAULT) {
		format = isSigned ? INTEGER_FORMAT_SIGNED : INTEGER_FORMAT_UNSIGNED;
	}

	format = GetFormatForTypeAndFormat(value.Type(), format, formatString,
		sizeof(formatString));

	// format the value
	switch (format) {
		case INTEGER_FORMAT_SIGNED:
			snprintf(buffer, bufferSize, formatString,
				value.Type() == B_INT8_TYPE ? value.ToInt8() :
					value.Type() == B_INT16_TYPE ? value.ToInt16() :
						value.Type() == B_INT32_TYPE ? value.ToInt32() :
							value.ToInt64());
			break;
		case INTEGER_FORMAT_UNSIGNED:
			snprintf(buffer, bufferSize, formatString,
				value.Type() == B_INT8_TYPE ? value.ToUInt8() :
					value.Type() == B_INT16_TYPE ? value.ToUInt16() :
						value.Type() == B_INT32_TYPE ? value.ToUInt32() :
							value.ToUInt64());
			break;
		case INTEGER_FORMAT_HEX_8:
			snprintf(buffer, bufferSize, "%#x", (uint8)value.ToUInt64());
			break;
		case INTEGER_FORMAT_HEX_16:
			snprintf(buffer, bufferSize, "%#x", (uint16)value.ToUInt64());
			break;
		case INTEGER_FORMAT_HEX_32:
			snprintf(buffer, bufferSize, "%#lx", (uint32)value.ToUInt64());
			break;
		case INTEGER_FORMAT_HEX_64:
		default:
			snprintf(buffer, bufferSize, "%#llx", value.ToUInt64());
			break;
	}

	return true;
}
