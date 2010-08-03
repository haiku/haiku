/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <sys/time.h>
#include <syscalls.h>


int
gettimeofday(struct timeval *tv, void *tz)
{
	if (tv != NULL) {
		bigtime_t usecs = real_time_clock_usecs();

		tv->tv_sec = usecs / 1000000;
		tv->tv_usec = usecs % 1000000;
	}

	// struct timezone (tz) has been deprecated since long and its exact
	// semantics are a bit unclear, so we need not bother to deal with it

	return 0;
}
