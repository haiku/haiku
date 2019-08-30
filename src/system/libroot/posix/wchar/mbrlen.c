/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
 
#include <wchar_private.h>
 
 
 size_t
__mbrlen(const char* s, size_t n, mbstate_t* ps)
 {
	if (ps == NULL) {
		static mbstate_t internalMbState;
		ps = &internalMbState;
	}
 
	return __mbrtowc(NULL, s, n, ps);
 }


B_DEFINE_WEAK_ALIAS(__mbrlen, mbrlen);
