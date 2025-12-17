/*
 * Copyright 2010-2024 Haiku Inc. All rights reserved.
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "StringForSize.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>


namespace BPrivate {


const char*
string_for_size(double size, char* string, size_t stringSize)
{
	const char* kFormats[] = {
		"%.0g B", "%.2g KiB", "%.2g MiB", "%.2g GiB", "%.2g TiB"
	};

	size_t index = 0;
	while (index < B_COUNT_OF(kFormats) - 1 && size >= 1024.0) {
		size /= 1024.0;
		index++;
	}

	snprintf(string, stringSize, kFormats[index], size);

	return string;
}


int64
parse_size(const char* sizeString)
{
	int64 parsedSize = -1;
	char* end;
	parsedSize = strtoll(sizeString, &end, 0);
	if (end != sizeString && parsedSize > 0) {
		int64 rawSize = parsedSize;
		switch (tolower(*end)) {
			case 't':
				parsedSize *= 1024;
			case 'g':
				parsedSize *= 1024;
			case 'm':
				parsedSize *= 1024;
			case 'k':
				parsedSize *= 1024;
				end++;
				break;
			case '\0':
				break;
			default:
				parsedSize = -1;
				break;
		}

		// Check for overflow
		if (parsedSize > 0 && rawSize > parsedSize)
			parsedSize = -1;
	}

	return parsedSize;
}


}	// namespace BPrivate

