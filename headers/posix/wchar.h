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
typedef int wctype_t;

typedef struct {
        int             __count;
        wint_t  __value;
} mbstate_t;

#include <size_t.h>

extern wint_t	fgetwc(FILE *);
extern wchar_t	*fgetws(wchar_t *, int, FILE *);
extern wint_t	fputwc(wchar_t, FILE *);
extern int	fputws(const wchar_t *, FILE *);
extern wint_t	getwc(FILE *);
extern wint_t	getwchar(void);

extern int	iswalnum(wint_t);
extern int	iswalpha(wint_t);
extern int	iswcntrl(wint_t);
extern int	iswdigit(wint_t);
extern int	iswgraph(wint_t);
extern int	iswlower(wint_t);
extern int	iswprint(wint_t);
extern int	iswpunct(wint_t);
extern int	iswspace(wint_t);
extern int	iswupper(wint_t);
extern int	iswxdigit(wint_t);
extern int 	iswctype(wint_t, wctype_t);

extern wint_t	putwc(wchar_t, FILE *);
extern wint_t	putwchar(wchar_t);

extern size_t 	mbrlen(const char *s, size_t n, mbstate_t *ps);
extern size_t 	mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps);
extern size_t	mbsrtowcs(wchar_t *dst, const char **src, size_t len, 
					mbstate_t *ps);
extern int		mbsinit(const mbstate_t *);

extern wint_t	towlower(wint_t);
extern wint_t	towupper(wint_t);
extern wint_t	ungetwc(wint_t, FILE *);

extern size_t   wcrtomb(char *, wchar_t, mbstate_t *);
extern wchar_t	*wcscat(wchar_t *, const wchar_t *);
extern wchar_t	*wcschr(const wchar_t *, wchar_t);
extern int      wcscmp(const wchar_t *ws1, const wchar_t *ws2);
extern int      wcscoll(const wchar_t *ws1, const wchar_t *ws2);
extern wchar_t	*wcscpy(wchar_t *, const wchar_t *);
extern size_t	wcscspn(const wchar_t *, const wchar_t *);
extern size_t	wcsftime(wchar_t *, size_t, const wchar_t *,
					const struct tm *);
extern size_t	wcslen(const wchar_t *);
extern wchar_t 	*wcsncat(wchar_t *, const wchar_t *, size_t);
extern int		wcsncmp(const wchar_t *, const wchar_t *, size_t);
extern wchar_t	*wcsncpy(wchar_t *, const wchar_t *, size_t);
extern wchar_t	*wcspbrk(const wchar_t *, const wchar_t *);
extern wchar_t	*wcsrchr(const wchar_t *, wchar_t);
extern size_t   wcsrtombs(char *dst, const wchar_t **src, size_t len,
					mbstate_t *ps);
extern size_t	wcsspn(const wchar_t *, const wchar_t *);
extern double	wcstod(const wchar_t *, wchar_t **);
extern wchar_t	*wcstok(wchar_t *, const wchar_t *, wchar_t **);
extern long int	wcstol(const wchar_t *, wchar_t **, int);
extern unsigned long int	wcstoul(const wchar_t *, wchar_t **, int);
extern wchar_t	*wcswcs(const wchar_t *, const wchar_t *);
extern int		wcswidth(const wchar_t *, size_t);
extern size_t	wcsxfrm(wchar_t *, const wchar_t *, size_t);
extern wctype_t	wctype(const char *);
extern int		wcwidth(wchar_t);

#define WEOF ((wint_t)(-1))

#endif /* _WCHAR_H */
