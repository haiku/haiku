/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <unistd.h>
#include <syscalls.h>
#include <errno.h>


#define RETURN_AND_SET_ERRNO(err) \
	if (err < 0) { \
		errno = err; \
		return -1; \
	} \
	return err;


int
truncate(const char *path, off_t newSize)
{
	struct stat stat;
	status_t status;

	stat.st_size = newSize;
	status = sys_write_stat(-1, path, true, &stat, FS_WRITE_STAT_SIZE);

	RETURN_AND_SET_ERRNO(status);
}


int
ftruncate(int fd, off_t newSize)
{
	struct stat stat;
	status_t status;

	stat.st_size = newSize;
	status = sys_write_stat(fd, NULL, true, &stat, FS_WRITE_STAT_SIZE);

	RETURN_AND_SET_ERRNO(status);
}

