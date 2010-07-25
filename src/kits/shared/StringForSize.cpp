/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "StringForSize.h"

#include <stdio.h>

#include <Catalog.h>
#include <LocaleBackend.h>
using BPrivate::gLocaleBackend;
using BPrivate::LocaleBackend;


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "StringForSize"


namespace BPrivate {


const char*
string_for_size(double size, char* string, size_t stringSize)
{
	// we need to translate some strings, and in order to do so, we need
	// to use the LocaleBackend to reache liblocale.so
	if (gLocaleBackend == NULL)
		LocaleBackend::LoadBackend();
	double kib = size / 1024.0;
	if (kib < 1.0) {
		const char* trKey = B_TRANSLATE_MARK("%d bytes");
		snprintf(string, stringSize, gLocaleBackend->GetString(trKey),
			(int)size);
		return string;
	}
	double mib = kib / 1024.0;
	if (mib < 1.0) {
		const char* trKey = B_TRANSLATE_MARK("%3.2f KiB");
		snprintf(string, stringSize, gLocaleBackend->GetString(trKey), kib);
		return string;
	}
	double gib = mib / 1024.0;
	if (gib < 1.0) {
		const char* trKey = B_TRANSLATE_MARK("%3.2f MiB");
		snprintf(string, stringSize, gLocaleBackend->GetString(trKey), mib);
		return string;
	}
	double tib = gib / 1024.0;
	if (tib < 1.0) {
		const char* trKey = B_TRANSLATE_MARK("%3.2f GiB");
		snprintf(string, stringSize, gLocaleBackend->GetString(trKey), gib);
		return string;
	}
	const char* trKey = B_TRANSLATE_MARK("%.2f TiB");
	snprintf(string, stringSize, gLocaleBackend->GetString(trKey), tib);
	return string;
}


}	// namespace BPrivate

