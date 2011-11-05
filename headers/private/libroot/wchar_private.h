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


extern wint_t	__btowc(int);

extern size_t 	__mbrlen(const char *s, size_t n, mbstate_t *ps);
extern size_t 	__mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps);
extern int		__mbsinit(const mbstate_t *);
extern size_t	__mbsrtowcs(wchar_t *dst, const char **src, size_t len,
					mbstate_t *ps);

extern size_t   __wcrtomb(char *, wchar_t, mbstate_t *);
extern wchar_t	*__wcscat(wchar_t *, const wchar_t *);
extern wchar_t	*__wcschr(const wchar_t *, wchar_t);
extern int      __wcscmp(const wchar_t *ws1, const wchar_t *ws2);
extern int      __wcscoll(const wchar_t *ws1, const wchar_t *ws2);
extern wchar_t	*__wcscpy(wchar_t *, const wchar_t *);
extern size_t	__wcscspn(const wchar_t *, const wchar_t *);
extern wchar_t	*__wcsdup(const wchar_t *);
extern size_t	__wcsftime(wchar_t *, size_t, const wchar_t *,
					const struct tm *);
extern size_t	__wcslen(const wchar_t *);
extern wchar_t 	*__wcsncat(wchar_t *, const wchar_t *, size_t);
extern int		__wcsncmp(const wchar_t *, const wchar_t *, size_t);
extern wchar_t	*__wcsncpy(wchar_t *, const wchar_t *, size_t);
extern wchar_t	*__wcspbrk(const wchar_t *, const wchar_t *);
extern wchar_t	*__wcsrchr(const wchar_t *, wchar_t);
extern size_t   __wcsrtombs(char *dst, const wchar_t **src, size_t len,
					mbstate_t *ps);
extern size_t	__wcsspn(const wchar_t *, const wchar_t *);
extern wchar_t	*__wcsstr(const wchar_t *, const wchar_t *);
extern double	__wcstod(const wchar_t *, wchar_t **);
extern float	__wcstof(const wchar_t *, wchar_t **);
extern wchar_t	*__wcstok(wchar_t *, const wchar_t *, wchar_t **);
extern long		__wcstol(const wchar_t *, wchar_t **, int);
extern long double			__wcstold(const wchar_t *, wchar_t **);
extern long long			__wcstoll(const wchar_t *, wchar_t **, int);
extern unsigned long		__wcstoul(const wchar_t *, wchar_t **, int);
extern unsigned long long	__wcstoull(const wchar_t *, wchar_t **, int);
extern wchar_t	*__wcswcs(const wchar_t *, const wchar_t *);
extern int		__wcswidth(const wchar_t *, size_t);
extern size_t	__wcsxfrm(wchar_t *, const wchar_t *, size_t);
extern int		__wctob(wint_t);
extern int		__wcwidth(wchar_t);
extern wchar_t	*__wmemchr(const wchar_t *, wchar_t, size_t);
extern int		__wmemcmp(const wchar_t *, const wchar_t *, size_t);
extern wchar_t	*__wmemcpy(wchar_t *, const wchar_t *, size_t);
extern wchar_t	*__wmemmove(wchar_t *, const wchar_t *, size_t);
extern wchar_t	*__wmemset(wchar_t *, wchar_t, size_t);


__END_DECLS


#endif	// _LIBROOT_WCHAR_PRIVATE_H
