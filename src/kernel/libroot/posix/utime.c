/* 
** Copyright 2002-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <fs_interface.h>

#include <utime.h>
#include <time.h>
#include <errno.h>
#include <syscalls.h>


#define RETURN_AND_SET_ERRNO(err) \
	if (err < 0) { \
		errno = err; \
		return -1; \
	} \
	return err;


int
utime(const char *path, const struct utimbuf *times)
{
	struct stat stat;
	status_t status;

	if (times != NULL) {
		stat.st_atime = times->actime;
		stat.st_mtime = times->modtime;
	} else
		stat.st_atime = stat.st_mtime = time(NULL);

	status = _kern_write_path_stat(path, true, &stat, sizeof(struct stat),
		FS_WRITE_STAT_MTIME | FS_WRITE_STAT_ATIME);

	RETURN_AND_SET_ERRNO(status);
}

