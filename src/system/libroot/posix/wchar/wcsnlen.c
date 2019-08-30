/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <wchar_private.h>


size_t
__wcsnlen(const wchar_t* wcs, size_t maxLength)
{
	size_t length = 0;

	while (length < maxLength && *wcs++ != L'\0')
		length++;

	return length;
}


B_DEFINE_WEAK_ALIAS(__wcsnlen, wcsnlen);
