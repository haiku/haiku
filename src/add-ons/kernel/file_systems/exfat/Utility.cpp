/*
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2014 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, korli@users.berlios.de
 *		John Scipione, jscipione@gmail.com
 */


#include "Utility.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Errors.h>

#include "encodings.h"


status_t
get_volume_name(struct exfat_entry* entry, char* name, size_t length)
{
	if (entry == NULL || name == NULL)
		return B_BAD_VALUE;

	if (entry->type == EXFAT_ENTRY_TYPE_NOT_IN_USE)
		name = "";
	else if (entry->type == EXFAT_ENTRY_TYPE_LABEL) {
		// UCS-2 can encode codepoints in the range U+0000 to U+FFFF
		// UTF-8 needs at most 3 bytes to encode values in this range
		size_t utf8NameLength = entry->label.length * 3;
		if (length < utf8NameLength)
			return B_NAME_TOO_LONG;

		status_t result = unicode_to_utf8((const uchar*)entry->label.name,
			entry->label.length * 2, (uint8*)name, &utf8NameLength);
		if (result != B_OK)
			return result;
	} else
		return B_NAME_NOT_FOUND;

	return B_OK;
}


void
get_default_volume_name(off_t diskSize, char* name, size_t length)
{
	off_t divisor = 1ULL << 40;
	char unit = 'T';
	if (diskSize < divisor) {
		divisor = 1UL << 30;
		unit = 'G';
		if (diskSize < divisor) {
			divisor = 1UL << 20;
			unit = 'M';
		}
	}

	double size = double((10 * diskSize + divisor - 1) / divisor);
		// %g in the kernel does not support precision...

	snprintf(name, length, "%g%ciB ExFAT Volume", size / 10, unit);
}
