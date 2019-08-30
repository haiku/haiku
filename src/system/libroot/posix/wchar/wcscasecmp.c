/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <wctype.h>

#include <wchar_private.h>


int
__wcscasecmp(const wchar_t* wcs1, const wchar_t* wcs2)
{
	int cmp;

	for (;;) {
		cmp = towlower(*wcs1) - towlower(*wcs2++);
			/* note: won't overflow, since our wchar_t is guaranteed to never
			   have the highest bit set */

		if (cmp != 0 || *wcs1++ == L'\0')
			break;
	}

	return cmp;
}


B_DEFINE_WEAK_ALIAS(__wcscasecmp, wcscasecmp);
