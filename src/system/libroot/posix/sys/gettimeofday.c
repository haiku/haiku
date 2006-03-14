/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <sys/time.h>
#include <syscalls.h>


int
gettimeofday(struct timeval *tv, struct timezone *tz)
{
	if (tv != NULL) {
		bigtime_t usecs = real_time_clock_usecs();

		tv->tv_sec = usecs / 1000000;
		tv->tv_usec = usecs % 1000000;
	}

	if (tz != NULL) {
		time_t timezoneOffset;
		bool daylightSavingTime;
		_kern_get_timezone(&timezoneOffset, &daylightSavingTime);

		tz->tz_minuteswest = timezoneOffset;
		tz->tz_dsttime = daylightSavingTime;
	}

	return 0;
}
