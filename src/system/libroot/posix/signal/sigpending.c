/*
 * Copyright (c) 2005, Haiku Project. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author(s):
 *		Jérôme Duval
 */


#include <syscalls.h>

#include <errno.h>
#include <signal.h>


int
sigpending(sigset_t *set)
{
	int err = _kern_sigpending(set);
	if (err < B_OK) {
		errno = err;
		return -1;
	}
	return 0;
}

