/* 
** Copyright 2002-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
**
** Copyright 2001, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <syscalls.h>


int
creat(const char *path, mode_t mode)
{
	int status = _kern_create(path, O_CREAT | O_TRUNC | O_WRONLY, mode);
	
	if (status < 0) {
		errno = status;
		return -1;
	}
	return status;
}


int
open(const char *path, int oflags, ...)
{
	int status;
	int perms;
	va_list args;

	va_start(args, oflags);
	perms = va_arg(args,int);
	va_end(args);

	if (oflags & O_CREAT)
		status = _kern_create(path, oflags, perms);
	else
		status = _kern_open(-1, path, oflags);

	if (status < 0) {
		errno = status;
		return -1;
	}
	return status;
}
