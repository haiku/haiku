/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002, Sebastian Nozzi.
 *
 * Distributed under the terms of the MIT license.
 */


#include "addAttr.h"

#include <TypeConstants.h>
#include <Mime.h>

#include <fs_attr.h>

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


template<class Type>
ssize_t
writeAttrValue(int fd, const char *name, type_code type, Type value)
{
	ssize_t bytes = fs_write_attr(fd, name, type, 0, &value, sizeof(Type));
	if (bytes < 0)
		return errno;

	return bytes;
}


/**	Writes an attribute to a node, taking the type into account and
 *	converting the value accordingly
 *
 *	On success it will return the amount of bytes written
 *	On failure it returns an error code (negative number)
 */

static ssize_t
writeAttr(int fd, type_code type, const char *name, const char *value, size_t length)
{
	uint64 uint64value = 0;
	int64 int64value = 0;
	double floatValue = 0.0;

	// parse number input at once

	switch (type) {
		case B_BOOL_TYPE:
		case B_INT8_TYPE:
		case B_INT16_TYPE:
		case B_INT32_TYPE:
		case B_INT64_TYPE:
			int64value = strtoll(value, NULL, 0);
			break;

		case B_UINT8_TYPE:
		case B_UINT16_TYPE:
		case B_UINT32_TYPE:
		case B_UINT64_TYPE:
			uint64value = strtoull(value, NULL, 0);
			break;

		case B_FLOAT_TYPE:
		case B_DOUBLE_TYPE:
			floatValue = strtod(value, NULL);
			break;
	}

	switch (type) {
		case B_INT8_TYPE:
			return writeAttrValue<int8>(fd, name, type, (int8)int64value);
		case B_INT16_TYPE:
			return writeAttrValue<int16>(fd, name, type, (int16)int64value);
		case B_INT32_TYPE:
			return writeAttrValue<int32>(fd, name, type, (int32)int64value);
		case B_INT64_TYPE:
			return writeAttrValue<int64>(fd, name, type, int64value);

		case B_UINT8_TYPE:
			return writeAttrValue<uint8>(fd, name, type, (uint8)uint64value);
		case B_UINT16_TYPE:
			return writeAttrValue<uint16>(fd, name, type, (uint16)uint64value);
		case B_UINT32_TYPE:
			return writeAttrValue<uint32>(fd, name, type, (uint32)uint64value);
		case B_UINT64_TYPE:
			return writeAttrValue<uint64>(fd, name, type, uint64value);

		case B_FLOAT_TYPE:
			return writeAttrValue<float>(fd, name, type, (float)floatValue);
		case B_DOUBLE_TYPE:
			return writeAttrValue<double>(fd, name, type, (double)floatValue);

		case B_BOOL_TYPE:
		{
			uint8 boolValue = 0;

			if (!strcasecmp(value, "true") || !strcasecmp(value, "t")
				|| !strcasecmp(value, "on") || !strcasecmp(value, "enabled")
				|| (isdigit(value[0]) && int64value == 1))
				boolValue = 1;
			else if (!strcasecmp(value, "false") || !strcasecmp(value, "f")
				|| !strcasecmp(value, "off") || !strcasecmp(value, "disabled")
				|| (isdigit(value[0]) && int64value == 0))
				boolValue = 0;
			else
				return B_BAD_VALUE;

			return writeAttrValue<uint8>(fd, name, B_BOOL_TYPE, boolValue);
		}

		case B_STRING_TYPE:
		case B_MIME_STRING_TYPE:
		default:
			// For string, mime-strings and any other type we just write the value
			// Note that the trailing NULL is added. If a length was given, we write
			// the value directly, though.
			ssize_t bytes = fs_write_attr(fd, name, type, 0, value,
				length ? length : strlen(value) + 1);
			if (bytes < 0)
				return errno;

			return bytes;
	}
}


/**	Adds an attribute to a file for the given type, name and value
 *	Converts the value accordingly in case of numeric or boolean types
 *
 *	On success, it returns B_OK, or else an appropriate error code.
 */

status_t
addAttr(const char *file, type_code type, const char *name,
	const char *value, size_t length, bool resolveLinks)
{
	int fd = open(file, O_RDONLY | (resolveLinks ? 0 : O_NOTRAVERSE));
	if (fd < 0)
		return errno;

	fs_remove_attr(fd, name);
	ssize_t bytes = writeAttr(fd, type, name, value, length);

	close(fd);

	return bytes >= 0 ? B_OK : bytes;
}

