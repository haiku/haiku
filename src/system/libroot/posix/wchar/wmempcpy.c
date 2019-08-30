/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <string.h>

#include <wchar_private.h>


wchar_t*
__wmempcpy(wchar_t* dest, const wchar_t* src, size_t count)
{
	memcpy(dest, src, count * sizeof(wchar_t));

	return dest + count;
}


B_DEFINE_WEAK_ALIAS(__wmempcpy, wmempcpy);
