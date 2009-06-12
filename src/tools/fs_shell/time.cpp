/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 *
 * Distributed under the terms of the MIT License.
 */

#include "compatibility.h"

#include "fssh_os.h"
#include "fssh_time.h"

#include <time.h>
#include <sys/time.h>

#include <OS.h>


// #pragma mark - OS.h

#if 0

void
fssh_set_real_time_clock(uint32_t secs_since_jan1_1970)
{
}


fssh_status_t
fssh_set_timezone(char *timezone)
{
}

#endif // 0

uint32_t
fssh_real_time_clock(void)
{
	timeval tv;
	gettimeofday(&tv, NULL);

	return tv.tv_sec;
}


fssh_bigtime_t
fssh_real_time_clock_usecs(void)
{
	timeval tv;
	gettimeofday(&tv, NULL);

	return tv.tv_sec * 1000000LL + tv.tv_usec;
}


fssh_bigtime_t
fssh_system_time(void)
{
	return system_time();
}


// #pragma mark - time.h


fssh_time_t
fssh_time(fssh_time_t *timer)
{
	time_t result = time(NULL);
	if (timer)
		*timer = result;
	return result;
}
