/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <wchar_private.h>


int
__wmemcmp(const wchar_t* wcs1, const wchar_t* wcs2, size_t count)
{
	while (count-- > 0) {
		const wchar_t wc1 = *wcs1++;
		const wchar_t wc2 = *wcs2++;
		if (wc1 > wc2)
			return 1;
		else if (wc2 > wc1)
			return -1;
	}

	return 0;
}


B_DEFINE_WEAK_ALIAS(__wmemcmp, wmemcmp);
