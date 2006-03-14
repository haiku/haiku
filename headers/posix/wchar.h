/*
 * Copyright (C) 2002 Marcus Overhagen
 * All rights reserved.
 *
 * Distributed under the terms of the MIT license
 */
#ifndef _WCHAR_H
#define _WCHAR_H

#include <wchar_t.h>

#define _WINT_T
typedef unsigned int wint_t;

typedef struct {
        int             __count;
        wint_t  __value;
} mbstate_t;

#include <size_t.h>

size_t 	mbrlen(const char *s, size_t n, mbstate_t *ps);
size_t 	mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps);
size_t	mbsrtowcs(wchar_t *dst, const char **src, size_t len, 
			mbstate_t *ps);
int		mbsinit(const mbstate_t *);
size_t	wcrtomb(char *, wchar_t, mbstate_t *);
wchar_t	*wcschr(const wchar_t *ws, wchar_t wc);
int 	wcscmp(const wchar_t *ws1, const wchar_t *ws2);
int 	wcscoll(const wchar_t *ws1, const wchar_t *ws2);
size_t	wcsrtombs(char *dst, const wchar_t **src, size_t len,
			mbstate_t *ps);

#define WEOF ((wint_t)(-1))

#endif /* _WCHAR_H */
