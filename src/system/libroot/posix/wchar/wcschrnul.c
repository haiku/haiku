/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/

#include <wchar_private.h>


wchar_t*
__wcschrnul(const wchar_t* wcs, wchar_t wc)
{
	while (*wcs != L'\0' && *wcs != wc)
		wcs++;

	return (wchar_t*)wcs;
}


B_DEFINE_WEAK_ALIAS(__wcschrnul, wcschrnul);
