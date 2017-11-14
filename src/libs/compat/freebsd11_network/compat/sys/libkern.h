/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_LIBKERN_H_
#define _FBSD_COMPAT_SYS_LIBKERN_H_


#include <sys/cdefs.h>
#include <sys/types.h>


extern int random(void);
uint32_t arc4random(void);

static __inline int imax(int a, int b) { return (a > b ? a : b); }

extern int abs(int a);

#endif /* _FBSD_COMPAT_SYS_LIBKERN_H_ */
