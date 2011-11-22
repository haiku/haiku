/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/

#include <errno.h>

#include <wchar_private.h>


int
__mbtowc(wchar_t* pwc, const char* s, size_t n)
{
	static mbstate_t internalMbState;

	int result = mbrtowc(pwc, s, n, &internalMbState);
	if (result == -2) {
		errno = EILSEQ;
		result = -1;
	}

	return result;
}


B_DEFINE_WEAK_ALIAS(__mbtowc, mbtowc);
