/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <wchar_private.h>


wchar_t*
__wcpncpy(wchar_t* dest, const wchar_t* src, size_t n)
{
	const wchar_t* srcEnd = src + n;
	wchar_t* destEnd = dest + n;

	while (src < srcEnd && *src != L'\0')
		*dest++ = *src++;

	if (dest < destEnd) {
		while (--destEnd >= dest)
			*destEnd = L'\0';
	}

	return dest;
}


B_DEFINE_WEAK_ALIAS(__wcpncpy, wcpncpy);
