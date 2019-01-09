/*
 * Copyright 2019, Leorize <leorize+oss@disroot.org>. All rights reserved.
 * Distributed under the terms of the MIT license.
 */


#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif

#include <limits.h>
#include <sys/resource.h>
#include <unistd.h>

#include <sys/param.h> /* MAX(), MIN() */


/* setpriority() is used for the implementation as they share the same
 * restrictions as defined in POSIX.1-2008. However, some restrictions might not
 * be implemented by Haiku's setpriority(). */
int
nice(int incr)
{
	int priority = incr;

	/* avoids overflow by checking the bounds beforehand */
	if (priority > -(2 * NZERO - 1) && priority < (2 * NZERO - 1))
		priority += getpriority(PRIO_PROCESS, 0);

	priority = MAX(priority, -NZERO);
	priority = MIN(priority, NZERO - 1);

	return setpriority(PRIO_PROCESS, 0, priority) != -1 ? priority : -1;
}
