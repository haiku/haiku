/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <signal.h>

#include <errno.h>

#include <pthread_private.h>
#include <signal_defs.h>
#include <syscalls.h>
#include <syscall_utils.h>


int
sigsetmask(int mask)
{
	sigset_t set = mask;
	sigset_t oset;

	if (sigprocmask(SIG_SETMASK, &set, &oset) < 0)
		return -1;

	return (int)oset;
}


int
sigblock(int mask)
{
	sigset_t set = mask;
	sigset_t oset;

	if (sigprocmask(SIG_BLOCK, &set, &oset) < 0)
		return -1;

	return (int)oset;
}


int
pthread_sigqueue(pthread_t thread, int sig, const union sigval userValue)
{
	status_t error;
	if (signal < 0)
		RETURN_AND_SET_ERRNO(EINVAL);

	error = _kern_send_signal(thread->id, sig, &userValue,
		SIGNAL_FLAG_SEND_TO_THREAD | SIGNAL_FLAG_QUEUING_REQUIRED);
	if (error != B_OK) {
		// translate B_BAD_THREAD_ID/B_BAD_TEAM_ID to ESRCH
        if (error == B_BAD_THREAD_ID || error == B_BAD_TEAM_ID)
			error = ESRCH;
	}

	RETURN_AND_SET_ERRNO(error);
}
