/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>

#include <sys/time.h>
#include <string.h>


#define USEC_PER_SECOND 1000000


int
getitimer(int which, struct itimerval *value)
{
	// ToDo: implement me!
	return -1;
}


int
setitimer(int which, const struct itimerval *value, struct itimerval *oldValue)
{
	// ToDo: implement me properly!
	// We probably need a better internal set_alarm() implementation to do this

	bigtime_t interval = value->it_interval.tv_sec * USEC_PER_SECOND + value->it_interval.tv_usec;
	bigtime_t remaining;
	if (interval != 0)
		remaining = set_alarm(interval, B_PERIODIC_ALARM);
	else {
		bigtime_t timeout = value->it_value.tv_sec * USEC_PER_SECOND + value->it_value.tv_usec;
		if (timeout != 0)
			remaining = set_alarm(timeout, B_ONE_SHOT_RELATIVE_ALARM);
		else {
			// cancel alarm
			remaining = set_alarm(B_INFINITE_TIMEOUT, B_PERIODIC_ALARM);
		}
	}

	if (oldValue != NULL) {
		// Record the time left of any previous itimer
		oldValue->it_value.tv_sec = remaining / USEC_PER_SECOND;
		oldValue->it_value.tv_usec = remaining % USEC_PER_SECOND;
	}

	return 0;
}

