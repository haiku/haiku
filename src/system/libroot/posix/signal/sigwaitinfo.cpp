/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <signal.h>

#include <errno.h>
#include <pthread.h>

#include <syscall_utils.h>

#include <errno_private.h>
#include <time_private.h>
#include <syscalls.h>


int
sigwaitinfo(const sigset_t* set, siginfo_t* info)
{
	return sigtimedwait(set, info, NULL);
}


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
	bigtime_t timeoutMicros = 0;
	bool invalidTime = false;
	if (timeout != NULL) {
		if (!timespec_to_bigtime(*timeout, timeoutMicros))
			invalidTime = true;
		flags = B_RELATIVE_TIMEOUT;
	} else {
		flags = 0;
		timeoutMicros = 0;
	}

	status_t error = _kern_sigwait(set, info, flags, timeoutMicros);

	pthread_testcancel();

	if (error != B_OK && invalidTime)
		error = EINVAL;

	if (error != B_OK)
		RETURN_AND_SET_ERRNO(error);

	return info->si_signo;
}
