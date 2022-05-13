/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _OBSD_COMPAT_SYS_PARAM_H_
#define _OBSD_COMPAT_SYS_PARAM_H_


#include_next <sys/param.h>


/* #pragma mark - bit manipulation */

#define SET(t, f)	((t) |= (f))
#define CLR(t, f)	((t) &= ~(f))
#define ISSET(t, f)	((t) & (f))


#endif	/* _OBSD_COMPAT_SYS_PARAM_H_ */
