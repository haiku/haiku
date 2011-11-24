/*
** Copyright 2001, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <unistd.h>

#include <errno.h>

#include <errno_private.h>
#include <syscalls.h>


off_t
lseek(int fd, off_t pos, int whence)
{
	off_t result = _kern_seek(fd, pos, whence);
	if (result < 0) {
		__set_errno(result);
		return -1;
	}
	return result;
}
