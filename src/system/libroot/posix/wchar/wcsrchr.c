/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/

#include <wchar_private.h>


wchar_t*
__wcsrchr(const wchar_t* wcsIn, wchar_t wc)
{
	const wchar_t* wcs = wcsIn + wcslen(wcsIn);
	for (; wcs >= wcsIn; --wcs) {
		if (*wcs == wc)
			return (wchar_t*)wcs;
	}

	return NULL;
}


B_DEFINE_WEAK_ALIAS(__wcsrchr, wcsrchr);
