/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/* Kernel space storage for "errno", located in the thread structure
 * (user "errno" can't be changed from kernel internal POSIX calls)
 */

#include <errno.h>
#include "thread.h"


int *
_errnop(void)
{
	struct thread *thread = thread_get_current_thread();

	return &thread->kernel_errno;
}

