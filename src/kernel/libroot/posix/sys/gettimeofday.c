/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <sys/time.h>
#include <OS.h>


int 
gettimeofday(struct timeval *tv, struct timezone *tz)
{
	bigtime_t usecs;

	if (tv == NULL)
		return 0;

	usecs = real_time_clock_usecs();

	tv->tv_sec = usecs / 1000000;
	tv->tv_usec = usecs % 1000000;

	return 0;
}
