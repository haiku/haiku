/*
 * Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <errno.h>

#include <errno_private.h>
#include <LocaleBackend.h>
#include <wchar_private.h>


using BPrivate::Libroot::GetCurrentLocaleBackend;
using BPrivate::Libroot::LocaleBackend;
using BPrivate::Libroot::LocaleBackendData;


extern "C" size_t
__wcsxfrm(wchar_t* dest, const wchar_t* src, size_t destSize)
{
	LocaleBackend* backend = GetCurrentLocaleBackend();

	if (backend != NULL) {
		size_t outSize = 0;
		status_t status = backend->Wcsxfrm(dest, src, destSize, outSize);

		if (status != B_OK)
			__set_errno(EINVAL);

		return outSize;
	}

	return wcslcpy(dest, src, destSize);
}


B_DEFINE_WEAK_ALIAS(__wcsxfrm, wcsxfrm);


extern "C" size_t
__wcsxfrm_l(wchar_t* dest, const wchar_t* src, size_t destSize, locale_t l)
{
	LocaleBackendData* locale = (LocaleBackendData*)l;
	LocaleBackend* backend = locale->backend;

	if (backend != NULL) {
		size_t outSize = 0;
		status_t status = backend->Wcsxfrm(dest, src, destSize, outSize);

		if (status != B_OK)
			__set_errno(EINVAL);

		return outSize;
	}

	return wcslcpy(dest, src, destSize);
}


B_DEFINE_WEAK_ALIAS(__wcsxfrm_l, wcsxfrm_l);
