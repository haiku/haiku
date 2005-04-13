/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <stdio.h>
#include <syscalls.h>
#include <errno.h>


int
rename(const char *from, const char *to)
{
	int status = _kern_rename(-1, from, -1, to);
	if (status < B_OK) {
		errno = status;
		return -1;
	}

	return status;
}

