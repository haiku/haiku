/*
** Copyright 2001, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <unistd.h>
#include <syscalls.h>
#include <errno.h>

#include <errno_private.h>


int
usleep(unsigned useconds)
{
	int err;
	err = snooze_until(system_time() + (bigtime_t)(useconds), B_SYSTEM_TIMEBASE);
	if (err < 0) {
		__set_errno(err);
		return -1;
	}
	return 0;
}
