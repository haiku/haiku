/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <signal.h>

#include <OS.h>

#include <errno_private.h>
#include <syscalls.h>


int
kill(pid_t pid, int sig)
{
	status_t status;

	if (sig < 0) {
		__set_errno(EINVAL);
		return -1;
	}

	status = _kern_send_signal(pid, sig, NULL, 0);
	if (status != B_OK) {
		// translate B_BAD_THREAD_ID/B_BAD_TEAM_ID to ESRCH
		if (status == B_BAD_THREAD_ID || status == B_BAD_TEAM_ID)
			status = ESRCH;

		__set_errno(status);
		return -1;
	}

	return 0;
}
