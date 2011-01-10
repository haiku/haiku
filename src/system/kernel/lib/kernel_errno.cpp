/*
 * Copyright 2003-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "thread.h"

#include <errno.h>


/*!
	Kernel space storage for "errno", located in the thread structure
	(user "errno" can't be changed from kernel internal POSIX calls)
*/
int *
_errnop(void)
{
	Thread *thread = thread_get_current_thread();

	return &thread->kernel_errno;
}

