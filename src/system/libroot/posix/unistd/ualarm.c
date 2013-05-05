/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <unistd.h>
#include <sys/time.h>

#include <errno_private.h>
#include <syscalls.h>


useconds_t
ualarm(useconds_t usec, useconds_t interval)
{
	struct itimerval value, oldValue;

	value.it_value.tv_sec = usec / 1000000;
	value.it_value.tv_usec = usec % 1000000;
	value.it_interval.tv_sec = interval / 1000000;
	value.it_interval.tv_usec = interval % 1000000;

	if (setitimer(ITIMER_REAL, &value, &oldValue) < 0)
		return -1;

	return (oldValue.it_value.tv_sec * 1000000) + oldValue.it_value.tv_usec;
}
