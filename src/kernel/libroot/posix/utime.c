/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <utime.h>
#include <errno.h>
#include <syscalls.h>


#define RETURN_AND_SET_ERRNO(err) \
	if (err < 0) { \
		errno = err; \
		return -1; \
	} \
	return err;


int
utime(const char *path, const struct utimbuf *buffer)
{
	struct stat stat;
	status_t status;

	stat.st_atime = buffer->actime;
	stat.st_mtime = buffer->modtime;
	status = sys_write_path_stat(path, false, &stat, FS_WRITE_STAT_MTIME | FS_WRITE_STAT_ATIME);

	RETURN_AND_SET_ERRNO(status);
}

