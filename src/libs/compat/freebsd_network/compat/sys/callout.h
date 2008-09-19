/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_CALLOUT_H_
#define _FBSD_COMPAT_SYS_CALLOUT_H_

#include <sys/haiku-module.h>

#include <sys/mutex.h>
#include <sys/queue.h>


struct callout {
	struct net_timer	c_timer;
	void *				c_arg;
	void				(*c_func)(void *);
	struct mtx *		c_mtx;
	int					c_flags;
};

#define CALLOUT_MPSAFE	0x0001

void callout_init_mtx(struct callout *c, struct mtx *mutex, int flags);
int	callout_reset(struct callout *, int, void (*)(void *), void *);

#define	callout_drain(c)	_callout_stop_safe(c, 1)
#define	callout_stop(c)		_callout_stop_safe(c, 0)
int	_callout_stop_safe(struct callout *, int);

#define	callout_pending(c)	((c)->c_timer.due > 0)
#define	callout_active(c)	((c)->c_timer.due == -1)
	// TODO: there is currently no way to find out about this!


static inline void
callout_init(struct callout *c, int mpsafe)
{
	if (mpsafe)
		callout_init_mtx(c, NULL, 0);
	else
		callout_init_mtx(c, &Giant, 0);
}

#endif	/* _FBSD_COMPAT_SYS_CALLOUT_H_ */
