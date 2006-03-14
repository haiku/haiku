/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <sys/time.h>
#include <sys/timeb.h>


int
ftime(struct timeb *timeb)
{
	struct timezone	tz;
	struct timeval tv;

	if (timeb == NULL)
		return -1;

	gettimeofday(&tv, &tz);

	timeb->time = tv.tv_sec;
	timeb->millitm = tv.tv_usec / 1000UL;
	timeb->timezone = tz.tz_minuteswest;
	timeb->dstflag = tz.tz_dsttime;

	return 0;
}

