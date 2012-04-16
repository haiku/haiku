/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "StringForSize.h"

#include <stdio.h>

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
		const char* trKey = B_TRANSLATE_MARK("%d bytes");
		snprintf(string, stringSize, gSystemCatalog.GetString(trKey,
			B_TRANSLATION_CONTEXT), (int)size);
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


}	// namespace BPrivate

