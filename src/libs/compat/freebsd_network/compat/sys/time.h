/*
 * Copyright 2020, Haiku, Inc. All rights reserved.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_TIME_H_
#define _FBSD_COMPAT_SYS_TIME_H_


#include <posix/sys/time.h>

#include <sys/_timeval.h>
#include <sys/types.h>


#define TICKS_2_USEC(t) ((1000000*(bigtime_t)t) / hz)
#define USEC_2_TICKS(t) (((bigtime_t)t*hz) / 1000000)

#define TICKS_2_MSEC(t) ((1000*(bigtime_t)t) / hz)
#define MSEC_2_TICKS(t) (((bigtime_t)t*hz) / 1000)

#define time_uptime		USEC_2_TICKS(system_time())


int	ppsratecheck(struct timeval*, int*, int);


#endif /* _FBSD_COMPAT_SYS_TIME_H_ */
