/*
 * Copyright 2012-2024, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "StringForRate.h"

#include <stdio.h>

#include <Catalog.h>
#include <NumberFormat.h>
#include <StringFormat.h>
#include <SystemCatalog.h>


using BPrivate::gSystemCatalog;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "StringForRate"


namespace BPrivate {


const char*
string_for_rate(double rate, char* string, size_t stringSize)
{
	const char* kFormats[] = {
		B_TRANSLATE_MARK_COMMENT("{0, plural, one{%s byte/s} other{%s bytes/s}}",
			"units per second"),
		B_TRANSLATE_MARK_COMMENT("%s KiB/s", "units per second"),
		B_TRANSLATE_MARK_COMMENT("%s MiB/s", "units per second"),
		B_TRANSLATE_MARK_COMMENT("%s GiB/s", "units per second"),
		B_TRANSLATE_MARK_COMMENT("%s TiB/s", "units per second")
	};

	size_t index = 0;
	while (index < B_COUNT_OF(kFormats) - 1 && rate >= 1024.0) {
		rate /= 1024.0;
		index++;
	}

	BString format;
	BStringFormat formatter(
		gSystemCatalog.GetString(kFormats[index], B_TRANSLATION_CONTEXT, "units per second"));
	formatter.Format(format, rate);

	BString printedRate;
	BNumberFormat numberFormat;
	numberFormat.SetPrecision(index == 0 ? 0 : 2);
	numberFormat.Format(printedRate, rate);

	snprintf(string, stringSize, format.String(), printedRate.String());

	return string;
}


}	// namespace BPrivate

