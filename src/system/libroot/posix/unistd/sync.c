/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <unistd.h>
#include <syscalls.h>
#include <errno.h>


int
fsync(int fd)
{
	int status = _kern_fsync(fd);
	if (status < 0) {
		errno = status;
		status = -1;
	}

	return status;
}


int
sync(void)
{
	int status = _kern_sync();
	if (status < 0) {
		errno = status;
		status = -1;
	}

	return status;
}
