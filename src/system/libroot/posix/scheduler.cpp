/*
 * Copyright 2008, Andreas Färber, andreas.faerber@web.de
 * Distributed under the terms of the MIT license.
 */

#include <sched.h>

#include <syscalls.h>


int
sched_yield(void)
{
	_kern_thread_yield();
	return 0;
}

