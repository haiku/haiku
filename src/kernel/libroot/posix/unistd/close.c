/* 
** Copyright 2001, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <unistd.h>
#include <syscalls.h>
#include <errno.h>

int close(int fd)
{
	int retval = sys_close(fd);

	if (retval < 0) {
		errno = retval;
		retval = -1;
	}

	return retval;
}
