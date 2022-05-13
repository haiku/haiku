/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _OBSD_COMPAT_SYS_TIME_H_
#define _OBSD_COMPAT_SYS_TIME_H_


#include_next <sys/time.h>


static inline time_t
getuptime()
{
	struct timeval tv;
	getmicrouptime(&tv);
	return tv.tv_sec;
}


static inline void
USEC_TO_TIMEVAL(uint64_t us, struct timeval *tv)
{
	tv->tv_sec = us / 1000000;
	tv->tv_usec = us % 1000000;
}

static inline uint64_t
SEC_TO_NSEC(uint64_t sec)
{
	if (sec > UINT64_MAX / 1000000000ULL)
		return UINT64_MAX;
	return sec * 1000000000ULL;
}

static inline uint64_t
MSEC_TO_NSEC(uint64_t ms)
{
	if (ms > UINT64_MAX / 1000000ULL)
		return UINT64_MAX;
	return ms * 1000000ULL;
}


#endif	/* _OBSD_COMPAT_SYS_TIME_H_ */
