/*
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Manuel J. Petit. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdarg.h>
#include <unistd.h>

#include <errno_private.h>
#include <syscalls.h>
#include <syscall_utils.h>
#include <umask.h>


int
creat(const char *path, mode_t mode)
{
	RETURN_AND_SET_ERRNO_TEST_CANCEL(
		_kern_open(-1, path, O_CREAT | O_TRUNC | O_WRONLY, mode & ~__gUmask));
		// adapt the permissions as required by POSIX
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

	RETURN_AND_SET_ERRNO_TEST_CANCEL(_kern_open(-1, path, openMode, perms));
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

	RETURN_AND_SET_ERRNO_TEST_CANCEL(_kern_open(fd, path, openMode, perms));
}


int
fcntl(int fd, int op, ...)
{
	va_list args;
	va_start(args, op);
	size_t argument = va_arg(args, size_t);
	va_end(args);

	status_t error = _kern_fcntl(fd, op, argument);

	if (op == F_SETLKW)
		pthread_testcancel();

	RETURN_AND_SET_ERRNO(error);
}
