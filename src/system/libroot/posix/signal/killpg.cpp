/*
 * Copyright 2007, Vasilis Kaoutsis, kaoutsis@sch.gr.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <signal.h>

#include <errno_private.h>


int
killpg(pid_t processGroupID, int signal)
{
	if (processGroupID > 1)
		return kill(-processGroupID, signal);
	else {
		__set_errno(EINVAL);
		return -1;
	}
}
