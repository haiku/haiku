/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <errno_private.h>
#include <LocaleBackend.h>
#include <wchar_private.h>


using BPrivate::Libroot::gLocaleBackend;


extern "C" int
__wcscoll(const wchar_t* wcs1, const wchar_t* wcs2)
{
	if (gLocaleBackend != NULL) {
		int result = 0;
		status_t status = gLocaleBackend->Wcscoll(wcs1, wcs2, result);

		if (status != B_OK)
			__set_errno(EINVAL);

		return result;
	}

	return wcscmp(wcs1, wcs2);
}


B_DEFINE_WEAK_ALIAS(__wcscoll, wcscoll);
