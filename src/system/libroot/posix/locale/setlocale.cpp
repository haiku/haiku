/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <ctype.h>
#include <locale.h>
#include <strings.h>

#include <ErrnoMaintainer.h>

#include "LocaleBackend.h"
#include "LocaleInternal.h"


using BPrivate::Libroot::gGlobalLocaleBackend;
using BPrivate::Libroot::GetLocalesFromEnvironment;
using BPrivate::Libroot::LocaleBackend;


extern "C" char*
setlocale(int category, const char* locale)
{
	BPrivate::ErrnoMaintainer errnoMaintainer;

	if (category < 0 || category > LC_LAST)
		return NULL;

	if (locale == NULL) {
		// query the locale of the given category
		if (gGlobalLocaleBackend != NULL)
			return const_cast<char*>(gGlobalLocaleBackend->SetLocale(category, NULL));
		else
			return const_cast<char*>("POSIX");
	}

	// we may have to invoke SetLocale once for each category, so we use an
	// array to collect the locale per category
	const char* locales[LC_LAST + 1];
	for (int lc = 0; lc <= LC_LAST; lc++)
		locales[lc] = NULL;

	if (*locale == '\0')
		GetLocalesFromEnvironment(category, locales);
	else
		locales[category] = locale;

	if (gGlobalLocaleBackend == NULL) {
		// for any locale other than POSIX/C, we try to activate the ICU
		// backend
		bool needBackend = false;
		for (int lc = 0; lc <= LC_LAST; lc++) {
			if (locales[lc] != NULL && strcasecmp(locales[lc], "POSIX") != 0
					&& strcasecmp(locales[lc], "C") != 0) {
				needBackend = true;
				break;
			}
		}
		if (needBackend && LocaleBackend::LoadBackend() != B_OK)
			return NULL;
	}

	if (gGlobalLocaleBackend != NULL) {
		for (int lc = 0; lc <= LC_LAST; lc++) {
			if (locales[lc] != NULL) {
				locale = gGlobalLocaleBackend->SetLocale(lc, locales[lc]);
				if (lc == LC_ALL) {
					// skip the rest, LC_ALL overrides
					return const_cast<char*>(locale);
				}
			}
		}
		return const_cast<char*>(gGlobalLocaleBackend->SetLocale(category, NULL));
	}

	return const_cast<char*>("POSIX");
}
