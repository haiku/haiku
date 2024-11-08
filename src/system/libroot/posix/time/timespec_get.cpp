/*
 * Copyright 2022, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <time.h>

#include <OS.h>
#include <SupportDefs.h>

#include <time_private.h>


int
timespec_get(struct timespec* time, int base)
{
	if (base != TIME_UTC)
		return 0;

	bigtime_t microSeconds = real_time_clock_usecs();
	bigtime_to_timespec(microSeconds, *time);
	return base;
}
