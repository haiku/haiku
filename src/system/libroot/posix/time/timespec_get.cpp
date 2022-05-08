/*
 * Copyright 2022, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <time.h>

#include <OS.h>
#include <SupportDefs.h>


int
timespec_get(struct timespec* time, int base)
{
	if (base != TIME_UTC)
		return 0;
	bigtime_t microSeconds = real_time_clock_usecs();

	// set the result
	time->tv_sec = microSeconds / 1000000;
	time->tv_nsec = (microSeconds % 1000000) * 1000;
	return base;
}

