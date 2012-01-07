/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/

#include <wchar_private.h>


wchar_t*
__wcsncpy(wchar_t* destIn, const wchar_t* src, size_t n)
{
	wchar_t* dest = destIn;
	const wchar_t* srcEnd = src + n;
	const wchar_t* destEnd = dest + n;

	while (src < srcEnd && *src != L'\0')
		*dest++ = *src++;
	while (dest < destEnd)
		*dest++ = L'\0';

	return destIn;
}


B_DEFINE_WEAK_ALIAS(__wcsncpy, wcsncpy);
