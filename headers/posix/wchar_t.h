/*
 * Copyright (C) 2002 Marcus Overhagen
 * All rights reserved.
 *
 * Distributed under the terms of the MIT license
 */
#ifndef _WCHAR_T_H
#define _WCHAR_T_H

#if !defined(__cplusplus) || __GNUC__ < 3
typedef unsigned short wchar_t;
#endif

#define _WINT_T
typedef unsigned int wint_t;

typedef struct {
	int		__count;
	wint_t	__value;
} mbstate_t;

#endif	/* _WCHAR_T_H */

