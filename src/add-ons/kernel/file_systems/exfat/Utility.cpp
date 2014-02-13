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

#include "convertutf.h"


status_t
get_volume_name(struct exfat_entry* entry, char* name, size_t length)
{
	if (entry == NULL || name == NULL)
		return B_BAD_VALUE;

	if (entry->type == EXFAT_ENTRY_TYPE_NOT_IN_USE)
		name = "";
	else if (entry->type == EXFAT_ENTRY_TYPE_LABEL) {
		ssize_t utf8Length = utf16le_to_utf8(entry->volume_label.name,
			entry->volume_label.length, name, length);
		if (utf8Length < 0)
			return (status_t)utf8Length;
	} else
		return B_NAME_NOT_FOUND;

	return B_OK;
}


void
get_default_volume_name(off_t partitionSize, char* name, size_t length)
{
	off_t divisor = 1ULL << 40;
	char unit = 'T';
	if (partitionSize < divisor) {
		divisor = 1UL << 30;
		unit = 'G';
		if (partitionSize < divisor) {
			divisor = 1UL << 20;
			unit = 'M';
		}
	}

	double size = double((10 * partitionSize + divisor - 1) / divisor);
		// %g in the kernel does not support precision...

	snprintf(name, length, "%g%ciB ExFAT Volume", size / 10, unit);
}
