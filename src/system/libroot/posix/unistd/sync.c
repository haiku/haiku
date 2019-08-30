/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include <unistd.h>

#include <errno.h>
#include <pthread.h>

#include <syscall_utils.h>

#include <errno_private.h>
#include <syscalls.h>


int
fsync(int fd)
{
	RETURN_AND_SET_ERRNO_TEST_CANCEL(_kern_fsync(fd));
}


void
sync(void)
{
	_kern_sync();
}

