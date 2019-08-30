/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <wchar_private.h>


size_t
__wcscspn(const wchar_t* wcs, const wchar_t* rejectIn)
{
	const wchar_t* wcPointer = wcs;
	wchar_t wc;
	for (; (wc = *wcPointer) != L'\0'; ++wcPointer) {
		const wchar_t* reject;
		for (reject = rejectIn; *reject != L'\0'; ++reject) {
			if (*reject == wc)
				return wcPointer - wcs;
		}
	}

	return wcPointer - wcs;
}

B_DEFINE_WEAK_ALIAS(__wcscspn, wcscspn);
