/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <errno_private.h>
#include <LocaleBackend.h>
#include <wchar_private.h>


using BPrivate::Libroot::GetCurrentLocaleBackend;
using BPrivate::Libroot::LocaleBackend;
using BPrivate::Libroot::LocaleBackendData;


extern "C" int
__wcscoll(const wchar_t* wcs1, const wchar_t* wcs2)
{
	LocaleBackend* backend = GetCurrentLocaleBackend();

	if (backend != NULL) {
		int result = 0;
		status_t status = backend->Wcscoll(wcs1, wcs2, result);

		if (status != B_OK)
			__set_errno(EINVAL);

		return result;
	}

	return wcscmp(wcs1, wcs2);
}


B_DEFINE_WEAK_ALIAS(__wcscoll, wcscoll);


extern "C" int
__wcscoll_l(const wchar_t* wcs1, const wchar_t* wcs2, locale_t l)
{
	LocaleBackendData* locale = (LocaleBackendData*)l;
	LocaleBackend* backend = locale->backend;

	if (backend != NULL) {
		int result = 0;
		status_t status = backend->Wcscoll(wcs1, wcs2, result);

		if (status != B_OK)
			__set_errno(EINVAL);

		return result;
	}

	return wcscmp(wcs1, wcs2);
}


B_DEFINE_WEAK_ALIAS(__wcscoll_l, wcscoll_l);
