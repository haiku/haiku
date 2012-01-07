/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LIBROOT_WCHAR_PRIVATE_H
#define _LIBROOT_WCHAR_PRIVATE_H


#include <BeBuild.h>

#include <sys/cdefs.h>
#include <wchar.h>


__BEGIN_DECLS


extern wint_t	__btowc(int c);

extern int		__mblen(const char *string, size_t length);
extern size_t 	__mbrlen(const char *s, size_t n, mbstate_t *mbState);
extern size_t 	__mbrtowc(wchar_t *dest, const char *src, size_t srcLength,
					mbstate_t *mbState);
extern int		__mbsinit(const mbstate_t *mbState);
extern size_t	__mbsnrtowcs(wchar_t *dest, const char **src, size_t srcLength,
					size_t destLength, mbstate_t *mbState);
extern size_t	__mbsrtowcs(wchar_t *dest, const char **src, size_t destLength,
					mbstate_t *mbState);
extern size_t	__mbstowcs(wchar_t *dest, const char *src, size_t destLength);
extern int		__mbtowc(wchar_t *dest, const char *src, size_t destLength);

extern wchar_t	*__wcpcpy(wchar_t *dest, const wchar_t *src);
extern wchar_t	*__wcpncpy(wchar_t *dest, const wchar_t *src, size_t srcLength);
extern size_t   __wcrtomb(char *dest, wchar_t wc, mbstate_t *mbState);
extern int		__wcscasecmp(const wchar_t *wcs1, const wchar_t *wcs2);
extern wchar_t	*__wcscat(wchar_t *dest, const wchar_t *src);
extern wchar_t	*__wcschr(const wchar_t *wcs, wchar_t wc);
extern wchar_t	*__wcschrnul(const wchar_t *wcs, wchar_t wc);
extern int      __wcscmp(const wchar_t *wcs1, const wchar_t *wcs2);
extern int      __wcscoll(const wchar_t *wcs1, const wchar_t *wcs2);
extern wchar_t	*__wcscpy(wchar_t *dest, const wchar_t *src);
extern size_t	__wcscspn(const wchar_t *wcs, const wchar_t *reject);
extern wchar_t	*__wcsdup(const wchar_t *wcs);
extern size_t	__wcsftime(wchar_t *dest, size_t destLength,
					const wchar_t *format, const struct tm *time);
extern size_t	__wcslcat(wchar_t *dest, const wchar_t *src, size_t maxLength);
extern size_t	__wcslcpy(wchar_t *dest, const wchar_t *src, size_t maxLength);
extern size_t	__wcslen(const wchar_t *wcs);
extern int		__wcsncasecmp(const wchar_t *wcs1, const wchar_t *wcs2,
					size_t maxLength);
extern wchar_t 	*__wcsncat(wchar_t *dest, const wchar_t *src, size_t srcLength);
extern int		__wcsncmp(const wchar_t *wcs1, const wchar_t *wcs2,
					size_t length);
extern wchar_t	*__wcsncpy(wchar_t *dest, const wchar_t *src, size_t srcLength);
extern size_t	__wcsnlen(const wchar_t *wcs, size_t maxLength);
extern size_t   __wcsnrtombs(char *dest, const wchar_t **src, size_t srcLength,
					size_t destLength, mbstate_t *mbState);
extern wchar_t	*__wcspbrk(const wchar_t *wcs, const wchar_t *accept);
extern wchar_t	*__wcsrchr(const wchar_t *wcs, wchar_t wc);
extern size_t   __wcsrtombs(char *dest, const wchar_t **src, size_t destLength,
					mbstate_t *mbState);
extern size_t	__wcsspn(const wchar_t *wcs, const wchar_t *accept);
extern wchar_t	*__wcsstr(const wchar_t *haystack, const wchar_t *needle);
extern double	__wcstod(const wchar_t *wcs, wchar_t **endPtr);
extern float	__wcstof(const wchar_t *wcs, wchar_t **endPtr);
extern wchar_t	*__wcstok(wchar_t *wcs1, const wchar_t *wcs2, wchar_t **ptr);
extern long		__wcstol(const wchar_t *wcs, wchar_t **endPtr, int base);
extern long double	__wcstold(const wchar_t *wcs, wchar_t **endPtr);
extern long long	__wcstoll(const wchar_t *wcs, wchar_t **endPtr, int base);
extern unsigned long	__wcstoul(const wchar_t *wcs, wchar_t **endPtr,
							int base);
extern unsigned long long	__wcstoull(const wchar_t *wcs, wchar_t **endPtr,
								int base);
extern size_t	__wcstombs(char *string, const wchar_t *wcs, size_t maxSize);
extern int		__wctomb(char *string, wchar_t wc);
extern wchar_t	*__wcswcs(const wchar_t *haystack, const wchar_t *needle);
extern int		__wcswidth(const wchar_t *wcs, size_t length);
extern size_t	__wcsxfrm(wchar_t *dest, const wchar_t *src, size_t destLength);
extern int		__wctob(wint_t wc);
extern int		__wcwidth(wchar_t wc);
extern wchar_t	*__wmemchr(const wchar_t *wcs, wchar_t wc, size_t n);
extern int		__wmemcmp(const wchar_t *wcs1, const wchar_t *wcs2, size_t n);
extern wchar_t	*__wmemcpy(wchar_t *dest, const wchar_t *src, size_t n);
extern wchar_t	*__wmempcpy(wchar_t *dest, const wchar_t *src, size_t n);
extern wchar_t	*__wmemmove(wchar_t *dest, const wchar_t *src, size_t n);
extern wchar_t	*__wmemset(wchar_t *dest, wchar_t wc, size_t n);


__END_DECLS


#endif	// _LIBROOT_WCHAR_PRIVATE_H
