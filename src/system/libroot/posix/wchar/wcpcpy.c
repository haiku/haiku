/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/

#include <wchar_private.h>


wchar_t*
__wcpcpy(wchar_t* dest, const wchar_t* src)
{
	while (*src != L'\0')
		*dest++ = *src++;
	*dest = L'\0';

	return dest;
}


B_DEFINE_WEAK_ALIAS(__wcpcpy, wcpcpy);
