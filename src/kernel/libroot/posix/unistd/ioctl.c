/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <unistd.h>
#include <syscalls.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <errno.h>


#define RETURN_AND_SET_ERRNO(err) \
	if (err < 0) { \
		errno = err; \
		return -1; \
	} \
	return err;


int
ioctl(int fd, ulong cmd, ...)
{
	va_list args;
	int err;

	va_start(args, cmd);
	err = sys_ioctl(fd, cmd, args, IOCPARM_LEN(cmd));
	va_end(args);

	RETURN_AND_SET_ERRNO(err)
}

