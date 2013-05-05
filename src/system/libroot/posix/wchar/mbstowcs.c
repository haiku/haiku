/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/

#include <wchar_private.h>


size_t
__mbstowcs(wchar_t* pwc, const char* s, size_t n)
{
	static mbstate_t internalMbState;

	return __mbsrtowcs(pwc, &s, n, &internalMbState);
}


B_DEFINE_WEAK_ALIAS(__mbstowcs, mbstowcs);
