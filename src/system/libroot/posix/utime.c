/*
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <utime.h>

#include <errno.h>
#include <time.h>

#include <NodeMonitor.h>

#include <errno_private.h>
#include <syscalls.h>


#define RETURN_AND_SET_ERRNO(err) \
	if (err < 0) { \
		__set_errno(err); \
		return -1; \
	} \
	return err;


int
utime(const char *path, const struct utimbuf *times)
{
	struct stat stat;
	status_t status;

	if (times != NULL) {
		stat.st_atim.tv_sec = times->actime;
		stat.st_mtim.tv_sec = times->modtime;
		stat.st_atim.tv_nsec = stat.st_mtim.tv_nsec = 0;
	} else {
		bigtime_t now = real_time_clock_usecs();
		stat.st_atim.tv_sec = stat.st_mtim.tv_sec = now / 1000000;
		stat.st_atim.tv_nsec = stat.st_mtim.tv_nsec = (now % 1000000) * 1000;
	}

	status = _kern_write_stat(-1, path, true, &stat, sizeof(struct stat),
		B_STAT_MODIFICATION_TIME | B_STAT_ACCESS_TIME);

	RETURN_AND_SET_ERRNO(status);
}

