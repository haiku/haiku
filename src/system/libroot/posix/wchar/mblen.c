/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/
 
#include <wchar_private.h>
 
 
 int
__mblen(const char* s, size_t n)
 {
	static mbstate_t internalMbState;
	int rval;
 
 	if (s == NULL) {
		static const mbstate_t initial;

		internalMbState = initial;

		return 0;	// we do not support stateful converters
 	}

	rval = __mbrtowc(NULL, s, n, &internalMbState);

	if (rval == -1 || rval == -2)
		return -1;

	return rval;
 }


B_DEFINE_WEAK_ALIAS(__mblen, mblen);
