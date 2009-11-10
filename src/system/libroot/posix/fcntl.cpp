/*
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Manuel J. Petit. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#define B_ENABLE_INCOMPLETE_POSIX_AT_SUPPORT 1
	// make the *at() functions and AT_* macros visible

#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <syscalls.h>
#include <syscall_utils.h>
#include <umask.h>


int
creat(const char *path, mode_t mode)
{
	int status = _kern_open(-1, path, O_CREAT | O_TRUNC | O_WRONLY,
		mode & ~__gUmask);
		// adapt the permissions as required by POSIX
	if (status < 0) {
		errno = status;
		return -1;
	}
	return status;
}


int
open(const char *path, int openMode, ...)
{
	int perms = 0;
	if (openMode & O_CREAT) {
		va_list args;
		va_start(args, openMode);
		perms = va_arg(args, int) & ~__gUmask;
			// adapt the permissions as required by POSIX
		va_end(args);
	}

	int status = _kern_open(-1, path, openMode, perms);
	if (status < 0) {
		errno = status;
		return -1;
	}
	return status;
}


int
openat(int fd, const char *path, int openMode, ...)
{
	int perms = 0;
	if (openMode & O_CREAT) {
		va_list args;
		va_start(args, openMode);
		perms = va_arg(args, int) & ~__gUmask;
			// adapt the permissions as required by POSIX
		va_end(args);
	}

	int status = _kern_open(fd, path, openMode, perms);
	if (status < 0) {
		errno = status;
		return -1;
	}
	return status;
}


int
fcntl(int fd, int op, ...)
{
	va_list args;
	va_start(args, op);
	uint32 argument = va_arg(args, uint32);
	va_end(args);

	RETURN_AND_SET_ERRNO(_kern_fcntl(fd, op, argument));
}
