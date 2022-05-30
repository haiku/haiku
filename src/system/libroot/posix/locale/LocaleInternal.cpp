/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "LocaleInternal.h"

#include <locale.h>
#include <stdlib.h>
#include <strings.h>

#include <Debug.h>


namespace BPrivate {
namespace Libroot {


status_t
GetLocalesFromEnvironment(int category, const char** locales)
{
	if (category > LC_LAST) {
		return B_BAD_VALUE;
	}

	const char* locale = getenv("LC_ALL");
	if (locale != NULL && *locale != '\0')
		locales[category] = locale;
	else {
		// the order of the names must match the one specified in locale.h
		const char* const categoryNames[] = {
			"LC_ALL",
			"LC_COLLATE",
			"LC_CTYPE",
			"LC_MONETARY",
			"LC_NUMERIC",
			"LC_TIME",
			"LC_MESSAGES"
		};

		STATIC_ASSERT(B_COUNT_OF(categoryNames) == LC_LAST + 1);

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
			if (locale == NULL || *locale == '\0')
				locale = getenv("LANG");
			if (locale == NULL || *locale == '\0')
				locale = "POSIX";
			locales[lc] = locale;
			if (lastLocale != NULL && strcasecmp(locale, lastLocale) != 0)
				haveDifferentLocales = true;
		}
		if (!haveDifferentLocales) {
			// we can set all locales at once
			locales[LC_ALL] = locale;
		}
	}

	return B_OK;
}

}	// namespace Libroot
}	// namespace BPrivate
