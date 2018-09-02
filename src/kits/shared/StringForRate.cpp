/*
 * Copyright 2012-2018, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "StringForRate.h"

#include <stdio.h>

#include <StringFormat.h>
#include <SystemCatalog.h>

using BPrivate::gSystemCatalog;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "StringForRate"


namespace BPrivate {


const char*
string_for_rate(double rate, char* string, size_t stringSize)
{
	double kib = rate / 1024.0;
	if (kib < 1.0) {
		BString tmp;
		BStringFormat format(
			gSystemCatalog.GetString(B_TRANSLATE_MARK(
			"{0, plural, one{# byte/s} other{# bytes/s}}"),
			B_TRANSLATION_CONTEXT, "bytes per second"));
		format.Format(tmp, (int)rate);

		strlcpy(string, tmp.String(), stringSize);
		return string;
	}
	double mib = kib / 1024.0;
	if (mib < 1.0) {
		const char* trKey = B_TRANSLATE_MARK("%3.2f KiB/s");
		snprintf(string, stringSize, gSystemCatalog.GetString(trKey,
			B_TRANSLATION_CONTEXT, "KiB per second"), kib);
		return string;
	}
	double gib = mib / 1024.0;
	if (gib < 1.0) {
		const char* trKey = B_TRANSLATE_MARK("%3.2f MiB/s");
		snprintf(string, stringSize, gSystemCatalog.GetString(trKey,
			B_TRANSLATION_CONTEXT, "MiB per second"), mib);
		return string;
	}
	double tib = gib / 1024.0;
	if (tib < 1.0) {
		const char* trKey = B_TRANSLATE_MARK("%3.2f GiB/s");
		snprintf(string, stringSize, gSystemCatalog.GetString(trKey,
			B_TRANSLATION_CONTEXT, "GiB per second"), gib);
		return string;
	}
	const char* trKey = B_TRANSLATE_MARK("%.2f TiB/s");
	snprintf(string, stringSize, gSystemCatalog.GetString(trKey,
		B_TRANSLATION_CONTEXT, "TiB per second"), tib);
	return string;
}

}	// namespace BPrivate

