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

int		mbsinit(const mbstate_t *);
size_t	wcrtomb(char *, wchar_t, mbstate_t *);

#endif /* _WCHAR_H */
