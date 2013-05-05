/*
 * Copyright (c) 2005, Haiku Project. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author(s):
 *		Jérôme Duval
 */


#include <syscalls.h>

#include <errno.h>
#include <pthread.h>
#include <signal.h>

#include <errno_private.h>


int
pause(void)
{
	sigset_t mask;
	sigemptyset(&mask);

	__set_errno(_kern_sigsuspend(&mask));

	pthread_testcancel();

	return -1;
}

