/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <unistd.h>
#include <syscalls.h>
#include <errno.h>


uint
alarm(unsigned int sec)
{
	struct itimerval value, oldValue;

	value.it_interval.tv_sec = value.it_interval.tv_usec = 0;
	value.it_value.tv_sec = sec;
	value.it_value.tv_usec = 0;
	if (setitimer(ITIMER_REAL, &value, &oldValue) < 0)
		return -1;

	if (oldValue.it_value.tv_usec)
		oldValue.it_value.tv_sec++;

	return oldValue.it_value.tv_sec;
}

