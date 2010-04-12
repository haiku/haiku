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
		snprintf(string, stringSize,
			gLocaleBackend->GetString("%d bytes","StringForSize",""),
			(int)size);
		return string;
	}
	double mib = kib / 1024.0;
	if (mib < 1.0) {
		snprintf(string, stringSize,
			gLocaleBackend->GetString("%3.2f KiB","StringForSize",""), kib);
		return string;
	}
	double gib = mib / 1024.0;
	if (gib < 1.0) {
		snprintf(string, stringSize,
			gLocaleBackend->GetString("%3.2f MiB","StringForSize",""), mib);
		return string;
	}
	double tib = gib / 1024.0;
	if (tib < 1.0) {
		snprintf(string, stringSize,
			gLocaleBackend->GetString("%3.2f GiB","StringForSize",""), gib);
		return string;
	}
	snprintf(string, stringSize,
		gLocaleBackend->GetString("%.2f TiB","StringForSize",""), tib);
	return string;
}


}	// namespace BPrivate

