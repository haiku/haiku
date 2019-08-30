/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <wchar_private.h>


wchar_t*
__wcsncat(wchar_t* destIn, const wchar_t* src, size_t n)
{
	wchar_t* dest = destIn;
	const wchar_t* srcEnd = src + n;

	while (*dest != L'\0')
		dest++;
	while (src < srcEnd && *src != L'\0')
		*dest++ = *src++;
	*dest = L'\0';

	return destIn;
}


B_DEFINE_WEAK_ALIAS(__wcsncat, wcsncat);
