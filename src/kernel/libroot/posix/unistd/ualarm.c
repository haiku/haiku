/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <unistd.h>
#include <syscalls.h>
#include <errno.h>


uint
ualarm(uint usec, uint interval)
{
	struct itimerval value, oldValue;

	value.it_value.tv_sec = 0;
	value.it_value.tv_usec = usec;
	value.it_interval.tv_sec = 0;
	value.it_interval.tv_usec = interval;

	if (setitimer(ITIMER_REAL, &value, &oldValue) < 0)
		return -1;

	return (oldValue.it_value.tv_sec * 1000000) + oldValue.it_value.tv_usec;
}
