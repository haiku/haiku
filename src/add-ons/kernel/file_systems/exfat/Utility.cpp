/*
 * Copyright 2001-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2014 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		John Scipione, jscipione@gmail.com
 */


#include "Utility.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Errors.h>

#include "encodings.h"


status_t
volume_name(struct exfat_entry* entry, char* _name)
{
	if (entry == NULL || _name == NULL)
		return B_BAD_VALUE;

	if (entry->type == EXFAT_ENTRY_TYPE_NOT_IN_USE) {
		const char* untitled = "Untitled";
		size_t length = strlen(untitled);
		strncpy(_name, untitled, length);
		if (strlen(_name) < length)
			return B_NAME_TOO_LONG;
	} else if (entry->type == EXFAT_ENTRY_TYPE_LABEL) {
		// UCS-2 can encode codepoints in the range U+0000 to U+FFFF
		// UTF-8 needs at most 3 bytes to encode values in this range
		size_t utf8NameLength = entry->label.length * 3;
		memset(_name, 0, utf8NameLength + 1);
			// zero out the character array
		unicode_to_utf8((const uchar*)entry->label.name,
			entry->label.length * 2, (uint8*)_name, &utf8NameLength);
		if (strlen(_name) < utf8NameLength)
			return B_NAME_TOO_LONG;
	} else
		return B_NAME_NOT_FOUND;

	return B_OK;
}
