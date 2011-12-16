/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <errno.h>
#include <string.h>

#include <errno_private.h>
#include "LocaleBackend.h"


using BPrivate::Libroot::gLocaleBackend;


extern "C" int
strcoll(const char *a, const char *b)
{
	if (gLocaleBackend != NULL) {
		int result = 0;
		status_t status = gLocaleBackend->Strcoll(a, b, result);

		if (status != B_OK)
			__set_errno(EINVAL);

		return result;
	}

	return strcmp(a, b);
}
