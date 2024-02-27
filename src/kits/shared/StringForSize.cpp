/*
 * Copyright 2010-2024 Haiku Inc. All rights reserved.
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "StringForSize.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <NumberFormat.h>
#include <StringFormat.h>
#include <SystemCatalog.h>


using BPrivate::gSystemCatalog;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "StringForSize"


namespace BPrivate {


const char*
string_for_size(double size, char* string, size_t stringSize)
{
	BString printedSize;

	double value = size / 1024.0;
	if (value < 1024.0) {
		BStringFormat format(
			B_TRANSLATE_MARK_ALL("{0, plural, one{# byte} other{# bytes}}",
			B_TRANSLATION_CONTEXT, "size unit"));

		format.Format(printedSize, (int)size);
		strlcpy(string, gSystemCatalog.GetString(printedSize.String(), B_TRANSLATION_CONTEXT,
			"size unit"), stringSize);

		return string;
	}

	const char* kFormats[] = {
		B_TRANSLATE_MARK_ALL("%s KiB", B_TRANSLATION_CONTEXT, "size unit"),
		B_TRANSLATE_MARK_ALL("%s MiB", B_TRANSLATION_CONTEXT, "size unit"),
		B_TRANSLATE_MARK_ALL("%s GiB", B_TRANSLATION_CONTEXT, "size unit"),
		B_TRANSLATE_MARK_ALL("%s TiB", B_TRANSLATION_CONTEXT, "size unit")
	};

	size_t index = 0;
	while (index < B_COUNT_OF(kFormats) && value >= 1024.0) {
		value /= 1024.0;
		index++;
	}

	BNumberFormat numberFormat;
	numberFormat.SetPrecision(2);
	numberFormat.Format(printedSize, value);
	snprintf(string, stringSize, gSystemCatalog.GetString(kFormats[index], B_TRANSLATION_CONTEXT,
		"size unit"), printedSize.String());

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

