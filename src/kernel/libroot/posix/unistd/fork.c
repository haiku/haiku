/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <syscalls.h>

#include <unistd.h>
#include <errno.h>


pid_t
fork(void)
{
	thread_id thread = _kern_fork();
	if (thread < 0) {
		errno = thread;
		return -1;
	}

	// ToDo: initialize child
	// ToDo: atfork() support

	return thread;
}

