/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/

#include <string.h>

#include <wchar_private.h>


wchar_t*
__wmemcpy(wchar_t* dest, const wchar_t* src, size_t count)
{
	return memcpy(dest, src, count * sizeof(wchar_t));
}


B_DEFINE_WEAK_ALIAS(__wmemcpy, wmemcpy);
