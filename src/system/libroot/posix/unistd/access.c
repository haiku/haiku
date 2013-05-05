/*
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <unistd.h>

#include <errno_private.h>
#include <syscalls.h>
#include <syscall_utils.h>


int
access(const char* path, int accessMode)
{
	status_t status = _kern_access(-1, path, accessMode, false);

	RETURN_AND_SET_ERRNO(status);
}


int
faccessat(int fd, const char* path, int accessMode, int flag)
{
	status_t status = _kern_access(fd, path, accessMode,
		(flag & AT_EACCESS) != 0);

	RETURN_AND_SET_ERRNO(status);
}
