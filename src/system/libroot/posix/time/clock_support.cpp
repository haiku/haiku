/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <time.h>

#include <errno.h>
#include <pthread.h>
#include <sys/resource.h>
#include <unistd.h>

#include <OS.h>

#include <syscall_utils.h>

#include <syscalls.h>


int
clock_getres(clockid_t clockID, struct timespec* resolution)
{
	// check the clock ID
	switch (clockID) {
		case CLOCK_MONOTONIC:
		case CLOCK_REALTIME:
		case CLOCK_PROCESS_CPUTIME_ID:
		case CLOCK_THREAD_CPUTIME_ID:
			break;
		default:
			if (clockID < 0)
				RETURN_AND_SET_ERRNO(EINVAL);

			// For clock IDs we can't otherwise verify, try to get the time.
			if (clockID != getpid()) {
				timespec dummy;
				if (clock_gettime(clockID, &dummy) != 0)
					return -1;
			}
	}

	// currently resolution is always 1us
	if (resolution != NULL) {
		resolution->tv_sec = 0;
		resolution->tv_nsec = 1000;
	}

	return 0;
}


int
clock_gettime(clockid_t clockID, struct timespec* time)
{
	// get the time in microseconds
	bigtime_t microSeconds;

	switch (clockID) {
		case CLOCK_MONOTONIC:
			microSeconds = system_time();
			break;
		case CLOCK_REALTIME:
			microSeconds = real_time_clock_usecs();
			break;
		case CLOCK_PROCESS_CPUTIME_ID:
		case CLOCK_THREAD_CPUTIME_ID:
		default:
		{
			status_t error = _kern_get_clock(clockID, &microSeconds);
			if (error != B_OK)
				RETURN_AND_SET_ERRNO(error);
		}
	}

	// set the result
	time->tv_sec = microSeconds / 1000000;
	time->tv_nsec = (microSeconds % 1000000) * 1000;

	return 0;
}


int
clock_settime(clockid_t clockID, const struct timespec* time)
{
	// can't set the monotonic clock
	if (clockID == CLOCK_MONOTONIC)
		RETURN_AND_SET_ERRNO(EINVAL);

	// check timespec validity
	if (time->tv_sec < 0 || time->tv_nsec < 0 || time->tv_nsec >= 1000000000)
		RETURN_AND_SET_ERRNO(EINVAL);

	// convert to microseconds and set the clock
	bigtime_t microSeconds = (bigtime_t)time->tv_sec * 1000000
		+ time->tv_nsec / 1000;

	RETURN_AND_SET_ERRNO(_kern_set_clock(clockID, microSeconds));
}


int
clock_nanosleep(clockid_t clockID, int flags, const struct timespec* time,
	struct timespec* remainingTime)
{
	// convert time to microseconds (round up)
	if (time->tv_sec < 0 || time->tv_nsec < 0 || time->tv_nsec >= 1000000000)
		RETURN_AND_TEST_CANCEL(EINVAL);

	bigtime_t microSeconds = (bigtime_t)time->tv_sec * 1000000
		+ (time->tv_nsec + 999) / 1000;

	// get timeout flags
	uint32 timeoutFlags;
	if ((flags & TIMER_ABSTIME) != 0) {
		timeoutFlags = B_ABSOLUTE_TIMEOUT;

		// ignore remainingTime for absolute waits
		remainingTime = NULL;
	} else
		timeoutFlags = B_RELATIVE_TIMEOUT;

	// wait
	bigtime_t remainingMicroSeconds;
	status_t error = _kern_snooze_etc(microSeconds, clockID, timeoutFlags,
		remainingTime != NULL ? &remainingMicroSeconds : NULL);

	// If interrupted and this is a relative wait, compute and return the
	// remaining wait time.
	if (error == B_INTERRUPTED && remainingTime != NULL) {
		if (remainingMicroSeconds > 0) {
			remainingTime->tv_sec = remainingMicroSeconds / 1000000;
			remainingTime->tv_nsec = (remainingMicroSeconds % 1000000) * 1000;
		} else {
			// We were slow enough that the wait time passed anyway.
			error = B_OK;
		}
	}

	RETURN_AND_TEST_CANCEL(error);
}


int
clock_getcpuclockid(pid_t pid, clockid_t* _clockID)
{
	if (pid < 0)
		return ESRCH;

	// The CPU clock ID for a process is simply the team ID. For pid == 0 we're
	// supposed to return the current process' clock.
	if (pid == 0) {
		*_clockID = getpid();
		return 0;
	}

	// test-get the time to verify the team exists and we have permission
	bigtime_t microSeconds;
	status_t error = _kern_get_clock(pid, &microSeconds);
	if (error != B_OK) {
		// Since pid is > 0, B_BAD_VALUE always means a team with that ID
		// doesn't exist. Translate the error code accordingly.
		if (error == B_BAD_VALUE)
			return ESRCH;
		return error;
	}

	*_clockID = pid;
	return 0;
}
