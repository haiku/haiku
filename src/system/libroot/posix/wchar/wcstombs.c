/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/

#include <wchar_private.h>


size_t
__wcstombs(char* s, const wchar_t* pwcs, size_t n)
{
	static mbstate_t internalMbState;

	return __wcsrtombs(s, &pwcs, n, &internalMbState);
}


B_DEFINE_WEAK_ALIAS(__wcstombs, wcstombs);
