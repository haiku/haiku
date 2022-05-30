/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <wctype.h>

#include <wchar_private.h>


int
__wcsncasecmp(const wchar_t* wcs1, const wchar_t* wcs2, size_t count)
{
	int cmp = 0;

	while (count-- > 0) {
		cmp = towlower(*wcs1) - towlower(*wcs2++);
			/* note: won't overflow, since our wchar_t is guaranteed to never
			   have the highest bit set */

		if (cmp != 0 || *wcs1++ == L'\0')
			break;
	}

	return cmp;
}


B_DEFINE_WEAK_ALIAS(__wcsncasecmp, wcsncasecmp);


int
__wcsncasecmp_l(const wchar_t* wcs1, const wchar_t* wcs2, size_t count, locale_t locale)
{
	int cmp = 0;

	while (count-- > 0) {
		cmp = towlower_l(*wcs1, locale) - towlower_l(*wcs2++, locale);
			/* note: won't overflow, since our wchar_t is guaranteed to never
			   have the highest bit set */

		if (cmp != 0 || *wcs1++ == L'\0')
			break;
	}

	return cmp;
}


B_DEFINE_WEAK_ALIAS(__wcsncasecmp_l, wcsncasecmp_l);
