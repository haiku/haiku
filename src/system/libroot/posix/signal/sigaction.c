/*
 * Copyright 2002-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Daniel Reinhold (danielre@users.sf.net)
 */


#include <errno.h>
#include <signal.h>

#include <syscalls.h>


int
sigaction(int sig, const struct sigaction *action, struct sigaction *oldAction)
{
	status_t status = _kern_sigaction(sig, action, oldAction);
	if (status < B_OK) {
		errno = status;
		return -1;
	}

	return 0;
}

