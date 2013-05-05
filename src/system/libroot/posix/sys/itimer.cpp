/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <sys/time.h>

#include <errno.h>
#include <string.h>

#include <OS.h>

#include <errno_private.h>
#include <syscall_utils.h>

#include <user_timer_defs.h>

#include <time_private.h>


static bool
itimerval_to_itimerspec(const itimerval& val, itimerspec& spec)
{
	return timeval_to_timespec(val.it_value, spec.it_value)
		&& timeval_to_timespec(val.it_interval, spec.it_interval);
}


static void
itimerspec_to_itimerval(const itimerspec& spec, itimerval& val)
{
	timespec_to_timeval(spec.it_value, val.it_value);
	timespec_to_timeval(spec.it_interval, val.it_interval);
}


static bool
prepare_timer(__timer_t& timer, int which)
{
	switch (which) {
		case ITIMER_REAL:
			timer.SetTo(USER_TIMER_REAL_TIME_ID, -1);
			return true;
		case ITIMER_VIRTUAL:
			timer.SetTo(USER_TIMER_TEAM_USER_TIME_ID, -1);
			return true;
		case ITIMER_PROF:
			timer.SetTo(USER_TIMER_TEAM_TOTAL_TIME_ID, -1);
			return true;
	}

	return false;
}


// #pragma mark -


int
getitimer(int which, struct itimerval* value)
{
	// prepare the respective timer
	__timer_t timer;
	if (!prepare_timer(timer, which))
		RETURN_AND_SET_ERRNO(EINVAL);

	// let timer_gettime() do the job
	itimerspec valueSpec;
	if (timer_gettime(&timer, &valueSpec) != 0)
		return -1;

	// convert back to itimerval value
	itimerspec_to_itimerval(valueSpec, *value);

	return 0;
}


int
setitimer(int which, const struct itimerval* value, struct itimerval* oldValue)
{
	// prepare the respective timer
	__timer_t timer;
	if (!prepare_timer(timer, which))
		RETURN_AND_SET_ERRNO(EINVAL);

	// convert value to itimerspec
	itimerspec valueSpec;
	if (!itimerval_to_itimerspec(*value, valueSpec))
		RETURN_AND_SET_ERRNO(EINVAL);

	// let timer_settime() do the job
	itimerspec oldValueSpec;
	if (timer_settime(&timer, 0, &valueSpec,
			oldValue != NULL ? &oldValueSpec : NULL) != 0) {
		return -1;
	}

	// convert back to itimerval oldValue
	if (oldValue != NULL)
		itimerspec_to_itimerval(oldValueSpec, *oldValue);

	return 0;
}
