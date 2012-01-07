/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/

#include <wchar_private.h>


wchar_t*
__wcscpy(wchar_t* destIn, const wchar_t* src)
{
	wchar_t* dest = destIn;

	while ((*dest++ = *src++) != L'\0')
		;

	return destIn;
}


B_DEFINE_WEAK_ALIAS(__wcscpy, wcscpy);
