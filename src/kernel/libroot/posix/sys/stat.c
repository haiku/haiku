/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <sys/stat.h>
#include <syscalls.h>
#include <errno.h>


#define RETURN_AND_SET_ERRNO(err) \
	if (err < 0) { \
		errno = err; \
		return -1; \
	} \
	return err;


int
stat(const char *path, struct stat *stat)
{
	int status = sys_read_path_stat(path, true, stat);

	RETURN_AND_SET_ERRNO(status);
}


int
lstat(const char *path, struct stat *stat)
{
	int status = sys_read_path_stat(path, false, stat);

	RETURN_AND_SET_ERRNO(status);
}


int
fstat(int fd, struct stat *stat)
{
	int status = sys_read_stat(fd, stat);

	RETURN_AND_SET_ERRNO(status);
}
