/*
** Copyright 2001, Travis Geiselbrecht.
** Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
** All rights reserved. Distributed under the terms of the NewOS License.
*/


#include <wchar_private.h>


wchar_t *
__wcstok(wchar_t* wcs, const wchar_t* delim, wchar_t** savePtr)
{
	wchar_t *wcsBegin, *wcsEnd;

	if (wcs == NULL && savePtr == NULL)
		return NULL;

	wcsBegin  = wcs ? wcs : *savePtr;
	if (wcsBegin == NULL)
		return NULL;

	wcsBegin += wcsspn(wcsBegin, delim);
	if (*wcsBegin == '\0') {
		if (savePtr)
			*savePtr = NULL;
		return NULL;
	}

	wcsEnd = wcspbrk(wcsBegin, delim);
	if (wcsEnd && *wcsEnd != '\0')
		*wcsEnd++ = '\0';
	if (savePtr)
		*savePtr = wcsEnd;

	return wcsBegin;
}


B_DEFINE_WEAK_ALIAS(__wcstok, wcstok);
