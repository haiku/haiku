/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <signal.h>

#include <errno.h>
#include <pthread.h>

#include <syscall_utils.h>

#include <syscalls.h>


int
sigtimedwait(const sigset_t* set, siginfo_t* info,
	const struct timespec* timeout)
{
	// make info non-NULL to simplify things
	siginfo_t stackInfo;
	if (info == NULL)
		info = &stackInfo;

	// translate the timeout
	uint32 flags;
    bigtime_t timeoutMicros;
	if (timeout != NULL) {
		timeoutMicros = system_time();
		flags = B_ABSOLUTE_TIMEOUT;
	    timeoutMicros += (bigtime_t)timeout->tv_sec * 1000000
        	+ timeout->tv_nsec / 1000;
	} else {
		flags = 0;
		timeoutMicros = 0;
	}

	status_t error = _kern_sigwait(set, info, flags, timeoutMicros);

	pthread_testcancel();

	if (error != B_OK)
		RETURN_AND_SET_ERRNO(error);

	return info->si_signo;
}
