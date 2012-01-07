/*
 * Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <errno.h>

#include <errno_private.h>
#include <LocaleBackend.h>
#include <wchar_private.h>


using BPrivate::Libroot::gLocaleBackend;


extern "C" size_t
__wcsxfrm(wchar_t* dest, const wchar_t* src, size_t destSize)
{
	if (gLocaleBackend != NULL) {
		size_t outSize = 0;
		status_t status = gLocaleBackend->Wcsxfrm(dest, src, destSize, outSize);

		if (status != B_OK)
			__set_errno(EINVAL);

		return outSize;
	}

	return wcslcpy(dest, src, destSize);
}


extern "C"
B_DEFINE_WEAK_ALIAS(__wcsxfrm, wcsxfrm);
