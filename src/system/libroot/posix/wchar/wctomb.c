/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <wchar_private.h>


int
__wctomb(char* s, wchar_t wc)
{
	static mbstate_t internalMbState;

	return __wcrtomb(s, wc, &internalMbState);
}


B_DEFINE_WEAK_ALIAS(__wctomb, wctomb);
