/*
 * Copyright (C) 2002 Marcus Overhagen
 * All rights reserved.
 *
 * Distributed under the terms of the MIT license
 */
#ifndef _WCHAR_H
#define _WCHAR_H

#include <wchar_t.h>

#include <size_t.h>

int		mbsinit(const mbstate_t *);
size_t	wcrtomb(char *, wchar_t, mbstate_t *);

#endif /* _WCHAR_H */
