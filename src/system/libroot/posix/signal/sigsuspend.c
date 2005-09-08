/*
 *  Copyright (c) 2005, Haiku Project. All rights reserved.
 *  Distributed under the terms of the Haiku license.
 *
 *  Author(s):
 *  Jérôme Duval
 */

#include <errno.h>
#include <syscalls.h>
#include <signal.h>

int
sigsuspend(const sigset_t *mask)
{
	int err = _kern_sigsuspend(mask);
	if (err < B_OK) {
		errno = err;
		return -1;
	}
	return 0;
}

