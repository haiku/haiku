/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <stdio.h>
#include <syscalls.h>
#include <errno.h>


int
remove(const char *path)
{
	int status = _kern_unlink(path);
	if (status < B_OK) {
		errno = status;
		return -1;
	}

	return status;
}

