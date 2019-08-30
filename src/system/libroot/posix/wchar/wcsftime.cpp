/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <wchar_private.h>


extern "C" size_t
__wcsftime(wchar_t* wcs, size_t wcsSize, const wchar_t* format,
	const struct tm* time)
{
#error "not implemented";

	return cmp;
}


extern "C"
B_DEFINE_WEAK_ALIAS(__wcsftime, wcsftime);
