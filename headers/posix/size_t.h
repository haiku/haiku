/*
 * Copyright 2008-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SIZE_T_H_
#define _SIZE_T_H_


#include <stddef.h>


/* ATM we don't define ssize_t for GCC2 */
#if __GNUC__ > 2 && !defined(__ssize_t__)
#	define __ssize_t__
typedef long signed int	ssize_t;
#endif


#endif	/* _SIZE_T_H_ */
