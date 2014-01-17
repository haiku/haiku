/*
 * Copyright 2009, Michael Franz
 * Copyright 2008, Andreas Färber, andreas.faerber@web.de
 * Distributed under the terms of the MIT license.
 */


#include <errno.h>
#include <sched.h>

#include <OS.h>

#include <errno_private.h>
#include <syscalls.h>


int
sched_yield(void)
{
	_kern_thread_yield();
	return 0;
}


int
sched_get_priority_min(int policy)
{
	switch (policy) {
		case SCHED_RR:
			return B_FIRST_REAL_TIME_PRIORITY;

		case SCHED_OTHER:
			return B_LOW_PRIORITY;

		default:
			__set_errno(EINVAL);
			return -1;
	}
}


int
sched_get_priority_max(int policy)
{
	switch (policy) {
		case SCHED_RR:
			return B_REAL_TIME_PRIORITY;

		case SCHED_OTHER:
			return B_URGENT_DISPLAY_PRIORITY;

		default:
			__set_errno(EINVAL);
			return -1;
	}
}
