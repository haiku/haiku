/* 
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
	int status = sys_create(path, O_CREAT | O_TRUNC | O_WRONLY, mode);
	
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
		status = sys_create(path, oflags, perms);
	else
		status = sys_open(path, oflags);

	if (status < 0) {
		errno = status;
		return -1;
	}
	return status;
}
