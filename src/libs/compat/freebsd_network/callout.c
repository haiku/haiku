/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */

#include "device.h"

#include <compat/sys/callout.h>


static void
handle_callout(struct net_timer *timer, void *data)
{
	struct callout *c = data;
	struct mtx *c_mtx = c->c_mtx;

	if (c_mtx)
		mtx_lock(c_mtx);

	/* FreeBSD 6.2 uses THREAD_NO_SLEEPING/THREAD_SLEEPING_OK when calling the
	 * callback */

	c->c_func(c->c_arg);

	if (c_mtx)
		mtx_unlock(c_mtx);
}


void
callout_init_mtx(struct callout *c, struct mtx *mtx, int flags)
{
	gStack->init_timer(&c->c_timer, handle_callout, c);

	c->c_arg = NULL;
	c->c_func = NULL;
	c->c_mtx = mtx;
	c->c_flags = flags;
}


int
callout_reset(struct callout *c, int when, void (*func)(void *), void *arg)
{
	int canceled = gStack->cancel_timer(&c->c_timer) ? 1 : 0;

	c->c_func = func;
	c->c_arg = arg;

	gStack->set_timer(&c->c_timer, when);

	return canceled;
}


int
_callout_stop_safe(struct callout *c, int safe)
{
	return gStack->cancel_timer(&c->c_timer) ? 1 : 0;
}

