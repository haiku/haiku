/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <errno.h>
#include <string.h>

#include <errno_private.h>
#include "LocaleBackend.h"


using BPrivate::Libroot::GetCurrentLocaleBackend;
using BPrivate::Libroot::LocaleBackend;
using BPrivate::Libroot::LocaleBackendData;


extern "C" int
strcoll(const char *a, const char *b)
{
	LocaleBackend* backend = GetCurrentLocaleBackend();

	if (backend != NULL) {
		int result = 0;
		status_t status = backend->Strcoll(a, b, result);

		if (status != B_OK)
			__set_errno(EINVAL);

		return result;
	}

	return strcmp(a, b);
}


extern "C" int
strcoll_l(const char *a, const char *b, locale_t l)
{
	LocaleBackendData* locale = (LocaleBackendData*)l;
	LocaleBackend* backend = locale->backend;

	if (backend != NULL) {
		int result = 0;
		status_t status = backend->Strcoll(a, b, result);

		if (status != B_OK)
			__set_errno(EINVAL);

		return result;
	}

	return strcmp(a, b);
}
