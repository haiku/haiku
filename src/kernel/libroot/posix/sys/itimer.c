/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
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
	if (interval != 0)
		set_alarm(interval, B_PERIODIC_ALARM);
	else {
		bigtime_t timeout = value->it_value.tv_sec * USEC_PER_SECOND + value->it_value.tv_usec;
		if (timeout != 0)
			set_alarm(timeout, B_ONE_SHOT_RELATIVE_ALARM);
		else {
			// cancel alarm
			set_alarm(B_INFINITE_TIMEOUT, B_PERIODIC_ALARM);
		}
	}

	// whatever set_alarm() returns (not documented in the BeBook, and I haven't
	// investigated this yet), maybe we can maintain the oldValue with it.

	memset(oldValue, 0, sizeof(struct itimerval));

	return 0;
}

