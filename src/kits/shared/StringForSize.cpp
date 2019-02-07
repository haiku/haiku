/*
 * Copyright 2010-2019 Haiku Inc. All rights reserved.
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "StringForSize.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <StringFormat.h>
#include <SystemCatalog.h>

using BPrivate::gSystemCatalog;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "StringForSize"


namespace BPrivate {


const char*
string_for_size(double size, char* string, size_t stringSize)
{
	double kib = size / 1024.0;
	if (kib < 1.0) {
		const char* trKey = B_TRANSLATE_MARK(
			"{0, plural, one{# byte} other{# bytes}}");

		BString tmp;
		BStringFormat format(
			gSystemCatalog.GetString(trKey, B_TRANSLATION_CONTEXT));
		format.Format(tmp, (int)size);

		strlcpy(string, tmp.String(), stringSize);
		return string;
	}
	double mib = kib / 1024.0;
	if (mib < 1.0) {
		const char* trKey = B_TRANSLATE_MARK("%3.2f KiB");
		snprintf(string, stringSize, gSystemCatalog.GetString(trKey,
			B_TRANSLATION_CONTEXT), kib);
		return string;
	}
	double gib = mib / 1024.0;
	if (gib < 1.0) {
		const char* trKey = B_TRANSLATE_MARK("%3.2f MiB");
		snprintf(string, stringSize, gSystemCatalog.GetString(trKey,
			B_TRANSLATION_CONTEXT), mib);
		return string;
	}
	double tib = gib / 1024.0;
	if (tib < 1.0) {
		const char* trKey = B_TRANSLATE_MARK("%3.2f GiB");
		snprintf(string, stringSize, gSystemCatalog.GetString(trKey,
			B_TRANSLATION_CONTEXT), gib);
		return string;
	}
	const char* trKey = B_TRANSLATE_MARK("%.2f TiB");
	snprintf(string, stringSize, gSystemCatalog.GetString(trKey,
		B_TRANSLATION_CONTEXT), tib);
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

