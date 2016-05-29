/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "AttributeValue.h"

#include <stdio.h>

#include "AttributeClasses.h"


const char*
AttributeValue::ToString(char* buffer, size_t size)
{
	switch (attributeClass) {
		case ATTRIBUTE_CLASS_ADDRESS:
			snprintf(buffer, size, "%#" B_PRIx64, address);
			return buffer;
		case ATTRIBUTE_CLASS_BLOCK:
			snprintf(buffer, size, "(%p, %#" B_PRIx64 ")", block.data,
				block.length);
			return buffer;
		case ATTRIBUTE_CLASS_CONSTANT:
			snprintf(buffer, size, "%#" B_PRIx64, constant);
			return buffer;
		case ATTRIBUTE_CLASS_FLAG:
			snprintf(buffer, size, "%s", flag ? "true" : "false");
			return buffer;
		case ATTRIBUTE_CLASS_LINEPTR:
		case ATTRIBUTE_CLASS_LOCLISTPTR:
		case ATTRIBUTE_CLASS_MACPTR:
		case ATTRIBUTE_CLASS_RANGELISTPTR:
			snprintf(buffer, size, "%#" B_PRIx64, pointer);
			return buffer;
		case ATTRIBUTE_CLASS_REFERENCE:
			snprintf(buffer, size, "%p", reference);
			return buffer;
		case ATTRIBUTE_CLASS_STRING:
			snprintf(buffer, size, "\"%s\"", string);
			return buffer;

		default:
		case ATTRIBUTE_CLASS_UNKNOWN:
			return "<unknown>";
	}

	return buffer;
}
