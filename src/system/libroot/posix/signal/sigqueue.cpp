/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <signal.h>

#include <errno.h>

#include <syscall_utils.h>

#include <signal_defs.h>
#include <syscalls.h>


int
sigqueue(pid_t pid, int signal, const union sigval userValue)
{
	if (signal < 0)
		RETURN_AND_SET_ERRNO(EINVAL);

	if (pid <= 0)
		RETURN_AND_SET_ERRNO(ESRCH);

	status_t error = _kern_send_signal(pid, signal, &userValue,
		SIGNAL_FLAG_QUEUING_REQUIRED);
	if (error != B_OK) {
		// translate B_BAD_THREAD_ID/B_BAD_TEAM_ID to ESRCH
		if (error == B_BAD_THREAD_ID || error == B_BAD_TEAM_ID)
			error = ESRCH;
	}

	RETURN_AND_SET_ERRNO(error);
}
