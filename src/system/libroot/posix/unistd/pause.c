/*
 *  Copyright (c) 2005, Haiku Project. All rights reserved.
 *  Distributed under the terms of the Haiku license.
 *
 *  Author(s):
 *  Jérôme Duval
 */

#include <errno.h>
#include <signal.h>
#include <syscalls.h>
#include <unistd.h>

int
pause()
{
	sigset_t mask;
	sigemptyset(&mask);
	errno = _kern_sigsuspend(&mask);
	return -1;
}

