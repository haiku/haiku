/*
 * Copyright 2018, Haiku Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_COMPAT_TIME_H
#define _KERNEL_COMPAT_TIME_H


#include <time.h>


struct compat_timespec {
	uint32	tv_sec;			/* seconds */
	uint32	tv_nsec;		/* and nanoseconds */
};


#endif // _KERNEL_COMPAT_TIME_H
