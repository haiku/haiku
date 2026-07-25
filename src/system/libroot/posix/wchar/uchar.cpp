/*
 * Copyright 2026 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#include <uchar.h>
#include <wchar.h>


static mbstate_t c16rtombState;


extern "C" size_t
c16rtomb(char *dest, char32_t wc, mbstate_t *mbState)
{
	if (mbState == NULL)
		mbState = &c16rtombState;
	wchar_t tmp = (wchar_t)wc;
	return wcrtomb(dest, tmp, mbState);
}


static mbstate_t mbrtoc32State;


extern "C" size_t
mbrtoc32(char32_t *dest, const char *src, size_t srcLength, mbstate_t *mbState)
{
	if (mbState == NULL)
		mbState = &mbrtoc32State;
	return mbrtowc((wchar_t*)dest, src, srcLength, mbState);
}


static mbstate_t c32rtombState;


extern "C" size_t
c32rtomb(char *dest, char32_t wc, mbstate_t *mbState)
{
	if (mbState == NULL)
		mbState = &c32rtombState;
	return wcrtomb(dest, (wchar_t)wc, mbState);
}
