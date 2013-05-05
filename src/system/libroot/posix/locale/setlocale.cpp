/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <ctype.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

#include <ErrnoMaintainer.h>

#include "LocaleBackend.h"


using BPrivate::Libroot::gLocaleBackend;
using BPrivate::Libroot::LocaleBackend;


static status_t
GetLocalesFromEnvironment(int category, const char** locales)
{
	const char* locale = getenv("LC_ALL");
	if (locale && *locale)
		locales[category] = locale;
	else {
		// the order of the names must match the one specified in locale.h
		const char* categoryNames[] = {
			"LC_ALL",
			"LC_COLLATE",
			"LC_CTYPE",
			"LC_MONETARY",
			"LC_NUMERIC",
			"LC_TIME",
			"LC_MESSAGES"
		};
		int from, to;
		if (category == LC_ALL) {
			// we need to check each real category if all of them should be set
			from = 1;
			to = LC_LAST;
		} else
			from = to = category;
		bool haveDifferentLocales = false;
		locale = NULL;
		for (int lc = from; lc <= to; lc++) {
			const char* lastLocale = locale;
			locale = getenv(categoryNames[lc]);
			if (!locale || *locale == '\0')
				locale = getenv("LANG");
			if (!locale || *locale == '\0')
				locale = "POSIX";
			locales[lc] = locale;
			if (lastLocale && strcasecmp(locale, lastLocale) != 0)
				haveDifferentLocales = true;
		}
		if (!haveDifferentLocales) {
			// we can set all locales at once
			locales[LC_ALL] = locale;
		}
	}

	return B_OK;
}


extern "C" char*
setlocale(int category, const char* locale)
{
	BPrivate::ErrnoMaintainer errnoMaintainer;

	if (category < 0 || category > LC_LAST)
		return NULL;

	if (locale == NULL) {
		// query the locale of the given category
		if (gLocaleBackend != NULL)
			return const_cast<char*>(gLocaleBackend->SetLocale(category, NULL));
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

	if (!gLocaleBackend) {
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

	if (gLocaleBackend != NULL) {
		for (int lc = 0; lc <= LC_LAST; lc++) {
			if (locales[lc] != NULL) {
				locale = gLocaleBackend->SetLocale(lc, locales[lc]);
				if (lc == LC_ALL) {
					// skip the rest, LC_ALL overrides
					return const_cast<char*>(locale);
				}
			}
		}
		return const_cast<char*>(gLocaleBackend->SetLocale(category, NULL));
	}

	return const_cast<char*>("POSIX");
}
