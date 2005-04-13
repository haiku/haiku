/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <unistd.h>
#include <syscalls.h>
#include <errno.h>


int
dup(int fd)
{
	int status = _kern_dup(fd);
	if (status < 0) {
		errno = status;
		status = -1;
	}

	return status;
}


int
dup2(int oldFD, int newFD)
{
	int status = _kern_dup2(oldFD, newFD);
	if (status < 0) {
		errno = status;
		status = -1;
	}

	return status;
}
