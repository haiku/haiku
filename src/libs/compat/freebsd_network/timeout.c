/*
 * Copyright 2009, Colin Günther, coling@gmx.de
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <sys/callout.h>
#include <sys/mutex.h>


inline void
callout_init(struct callout *callout, int mpsafe)
{
	if (mpsafe)
		callout_init_mtx(callout, NULL, 0);
	else
		callout_init_mtx(callout, &Giant, 0);
}


int
callout_schedule(struct callout *callout, int to_ticks)
{
	return callout_reset(callout, to_ticks, callout->c_func, callout->c_arg);
}
