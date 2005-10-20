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
sigsuspend(const sigset_t *mask)
{
	errno = _kern_sigsuspend(mask);
	return -1;
}

