/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <NodeMonitor.h>

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

	status = _kern_write_stat(-1, path, true, &stat, sizeof(struct stat),
		B_STAT_MODIFICATION_TIME | B_STAT_ACCESS_TIME);

	RETURN_AND_SET_ERRNO(status);
}

