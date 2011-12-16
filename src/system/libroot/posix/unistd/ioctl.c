/*
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <unistd.h>
#include <syscalls.h>
#include <stdarg.h>
#include <errno.h>

#include <errno_private.h>


#define RETURN_AND_SET_ERRNO(err) \
	if (err < 0) { \
		__set_errno(err); \
		return -1; \
	} \
	return err;


int
ioctl(int fd, ulong cmd, ...)
{
	va_list args;
	void* argument;
	size_t size;
	int status;

	va_start(args, cmd);
	argument = va_arg(args, void*);
	size = va_arg(args, size_t);
	va_end(args);

	status = _kern_ioctl(fd, cmd, argument, size);

	RETURN_AND_SET_ERRNO(status)
}
