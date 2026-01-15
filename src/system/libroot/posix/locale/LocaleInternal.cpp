/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de
 * Copyright 2022, Trung Nguyen, trungnt282910@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "LocaleInternal.h"

#include <ctype.h>
#include <locale.h>
#include <stdlib.h>
#include <strings.h>

#include <tls.h>
#include <OS.h>
#include <Debug.h>

#include "LocaleData.h"


namespace BPrivate {
namespace Libroot {


status_t
GetLocalesFromEnvironment(int category, const char** locales)
{
	if (category > LC_LAST)
		return B_BAD_VALUE;

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
		if (from == 1 && to == LC_LAST && !haveDifferentLocales) {
			// we can set all locales at once
			locales[LC_ALL] = locale;
		}
	}

	return B_OK;
}


static void
DestroyThreadLocale(void* ptr)
{
	ThreadLocale* threadLocale = (ThreadLocale*)ptr;
	delete threadLocale;
}


ThreadLocale*
GetCurrentThreadLocale()
{
	ThreadLocale* threadLocale = (ThreadLocale*)tls_get(TLS_LOCALE_SLOT);
	if (threadLocale == NULL) {
		threadLocale = new ThreadLocale();
		threadLocale->threadLocaleInfo = NULL;
		threadLocale->ctype_b = __ctype_b;
		threadLocale->ctype_tolower = __ctype_tolower;
		threadLocale->ctype_toupper = __ctype_toupper;
		threadLocale->mb_cur_max = &__ctype_mb_cur_max;

		on_exit_thread(DestroyThreadLocale, threadLocale);
		tls_set(TLS_LOCALE_SLOT, threadLocale);
	}
	return threadLocale;
}


}	// namespace Libroot
}	// namespace BPrivate
