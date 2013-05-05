/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/

#include <wchar_private.h>


wchar_t*
__wmemchr(const wchar_t* dest, const wchar_t wc, size_t count)
{
	while (count-- > 0) {
		if (*dest == wc)
			return (wchar_t*)dest;
		++dest;
	}

	return NULL;
}


B_DEFINE_WEAK_ALIAS(__wmemchr, wmemchr);
