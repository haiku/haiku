/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "IntegerFormatter.h"

#include <stdio.h>

#include <TypeConstants.h>


/*static*/ bool
IntegerFormatter::FormatValue(const BVariant& value, integer_format format,
	char* buffer, size_t bufferSize)
{
	bool isSigned;
	if (!value.IsInteger(&isSigned))
		return false;

	if (format == INTEGER_FORMAT_DEFAULT)
		format = isSigned ? INTEGER_FORMAT_SIGNED : INTEGER_FORMAT_UNSIGNED;

	if (format == INTEGER_FORMAT_HEX_DEFAULT) {
		switch (value.Type()) {
			case B_INT8_TYPE:
			case B_UINT8_TYPE:
				format = INTEGER_FORMAT_HEX_8;
				break;
			case B_INT16_TYPE:
			case B_UINT16_TYPE:
				format = INTEGER_FORMAT_HEX_16;
				break;
			case B_INT32_TYPE:
			case B_UINT32_TYPE:
				format = INTEGER_FORMAT_HEX_32;
				break;
			case B_INT64_TYPE:
			case B_UINT64_TYPE:
			default:
				format = INTEGER_FORMAT_HEX_64;
				break;
		}
	}

	// format the value
	switch (format) {
		case INTEGER_FORMAT_SIGNED:
			snprintf(buffer, bufferSize, "%lld", value.ToInt64());
			break;
		case INTEGER_FORMAT_UNSIGNED:
			snprintf(buffer, bufferSize, "%llu", value.ToUInt64());
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
		case INTEGER_FORMAT_HEX_DEFAULT:
		default:
			snprintf(buffer, bufferSize, "%#llx", value.ToUInt64());
			break;
	}

	return true;
}
