/* 
** Copyright 2001, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <unistd.h>
#include <syscalls.h>


int
dup(int fd)
{
	int retval;

	retval= sys_dup(fd);

	if(retval< 0) {
		// set errno
	}

	return retval;
}
