/*
 * Copyright 2018 Haiku, Inc. All Right Reserved
 * Distributed under the terms of MIT license.
 */
#ifndef _UCHAR_H
#define _UCHAR_H

#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#else
typedef wchar_t char32_t; /* our wchar_t is utf32 */
typedef wchar_t char16_t;
#endif

#define __STD_UTF_32__ 1

/* We don't define __STD_UTF_16__, so the format of char16_t is unspecified */
static __inline size_t
c16rtomb(char *dest, char16_t wc, mbstate_t *mbState)
{
	return wcrtomb(dest, wc, mbState);
}
static __inline size_t
mbrtoc16(char16_t *dest, const char *src, size_t srcLength, mbstate_t *mbState)
{
	return mbrtowc(dest, src, srcLength, mbState);
}

static __inline size_t
c32rtomb(char *dest, char32_t wc, mbstate_t *mbState)
{
	return wcrtomb(dest, wc, mbState);
}
static __inline size_t
mbrtoc32(char32_t *dest, const char *src, size_t srcLength, mbstate_t *mbState)
{
	return mbrtowc(dest, src, srcLength, mbState);
}

#ifdef __cplusplus
}
#endif
#endif /* _UCHAR_H */
