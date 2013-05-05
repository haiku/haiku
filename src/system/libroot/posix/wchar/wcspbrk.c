/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/

#include <wchar_private.h>


wchar_t*
__wcspbrk(const wchar_t* wcs, const wchar_t* acceptIn)
{
	for (; *wcs != L'\0'; ++wcs) {
		const wchar_t* accept = acceptIn;
		for (; *accept != L'\0'; ++accept) {
			if (*accept == *wcs)
				return (wchar_t*)wcs;
		}
	}

	return NULL;
}


B_DEFINE_WEAK_ALIAS(__wcspbrk, wcspbrk);
