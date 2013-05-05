/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <sys/file.h>

#include <errno.h>
#include <pthread.h>

#include <errno_private.h>
#include <syscalls.h>


int
flock(int fd, int op)
{
	status_t status = _kern_flock(fd, op);

	pthread_testcancel();

	if (status < B_OK) {
		__set_errno(status);
		return -1;
	}

	return 0;
}

