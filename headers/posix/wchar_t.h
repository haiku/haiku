/*
 * Copyright (C) 2002 Marcus Overhagen
 * All rights reserved.
 *
 * Distributed under the terms of the MIT license
 */
#ifndef _WCHAR_T_H
#define _WCHAR_T_H

typedef unsigned short wchar_t;

#define _WINT_T
typedef unsigned int wint_t;

typedef struct {
	int		__count;
	wint_t	__value;
} mbstate_t;

#endif	/* _WCHAR_T_H */

