/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <wchar_private.h>


wchar_t*
__wmemset(wchar_t* destIn, const wchar_t wc, size_t count)
{
	wchar_t* dest = destIn;
	while (count-- > 0)
		*dest++ = wc;

	return destIn;
}


B_DEFINE_WEAK_ALIAS(__wmemset, wmemset);
