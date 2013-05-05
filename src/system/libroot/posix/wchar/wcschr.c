/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/

#include <wchar_private.h>


wchar_t*
__wcschr(const wchar_t* wcs, wchar_t wc)
{
	while (1) {
		if (*wcs == wc)
			return (wchar_t*)wcs;
		if (*wcs++ == L'\0')
			break;
	}

	return NULL;
}


B_DEFINE_WEAK_ALIAS(__wcschr, wcschr);
