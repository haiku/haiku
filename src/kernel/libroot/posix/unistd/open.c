/*
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Manuel J. Petit. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <syscalls.h>


extern mode_t __gUmask;
	// declared in sys/umask.c


int
creat(const char *path, mode_t mode)
{
	int status = _kern_open(-1, path, O_CREAT | O_TRUNC | O_WRONLY, mode & ~__gUmask);
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
	int status;
	int perms = 0;
	va_list args;

	if (openMode & O_CREAT) {
		va_start(args, openMode);
		perms = va_arg(args, int) & ~__gUmask;
			// adapt the permissions as required by POSIX
		va_end(args);
	}

	status = _kern_open(-1, path, openMode, perms);
	if (status < 0) {
		errno = status;
		return -1;
	}
	return status;
}
