/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <wchar_private.h>


size_t
__wcsspn(const wchar_t* wcs, const wchar_t* acceptIn)
{
	const wchar_t* wcPointer = wcs;
	wchar_t wc;
	for (; (wc = *wcPointer) != L'\0'; ++wcPointer) {
		const wchar_t* accept;
		for (accept = acceptIn; *accept != L'\0'; ++accept) {
			if (*accept == wc)
				break;
		}
		if (*accept == L'\0')
			break;
	}

	return wcPointer - wcs;
}


B_DEFINE_WEAK_ALIAS(__wcsspn, wcsspn);
