/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _OBSD_COMPAT_SYS_TIMEOUT_H_
#define _OBSD_COMPAT_SYS_TIMEOUT_H_


#include <sys/callout.h>


struct timeout {
	struct callout c;
};


static inline void
timeout_set(struct timeout *to, void (*fn)(void *), void *arg)
{
	callout_init_mtx(&to->c, &Giant, 0);
	callout_reset(&to->c, -1, fn, arg);
}


static inline int
timeout_pending(struct timeout *to)
{
	return callout_pending(&to->c);
}


static inline int
timeout_add_usec(struct timeout *to, int usec)
{
	return (callout_schedule(&to->c, USEC_2_TICKS(usec)) ? 0 : 1);
}


static inline int
timeout_add_msec(struct timeout *to, int msec)
{
	return timeout_add_usec(to, msec * 1000);
}


static inline int
timeout_add_sec(struct timeout *to, int sec)
{
	return timeout_add_msec(to, sec * 1000);
}


static inline int
timeout_del(struct timeout *to)
{
	return ((callout_stop(&to->c) == 1) ? 1 : 0);
}


#endif	/* _OBSD_COMPAT_SYS_TIMEOUT_H_ */
