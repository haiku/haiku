/* 
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>

#include <syscalls.h>


#define RETURN_AND_SET_ERRNO(err) \
	if (err < 0) { \
		errno = err; \
		return -1; \
	} \
	return err;


int
fcntl(int fd, int op, ...)
{
	status_t status;
	uint32 argument;
	va_list args;

	va_start(args, op);
	argument = va_arg(args, uint32);
	va_end(args);

	status = _kern_fcntl(fd, op, argument);

	RETURN_AND_SET_ERRNO(status)
}

