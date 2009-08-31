/*
 * Copyright 2006-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <sys/time.h>

#include <errno.h>

#include <NodeMonitor.h>

#include <syscalls.h>


#define RETURN_AND_SET_ERRNO(err) \
	if (err < 0) { \
		errno = err; \
		return -1; \
	} \
	return err;


int
utimes(const char* path, const struct timeval times[2])
{
	struct stat stat;
	status_t status;

	if (times != NULL) {
		stat.st_atim.tv_sec = times[0].tv_sec;
		stat.st_atim.tv_nsec = times[0].tv_usec * 1000;

		stat.st_mtim.tv_sec = times[1].tv_sec;
		stat.st_mtim.tv_nsec = times[1].tv_usec * 1000;
	} else {
		bigtime_t now = real_time_clock_usecs();
		stat.st_atim.tv_sec = stat.st_mtim.tv_sec = now / 1000000;
		stat.st_atim.tv_nsec = stat.st_mtim.tv_nsec = (now % 1000000) * 1000;
	}

	status = _kern_write_stat(-1, path, true, &stat, sizeof(struct stat),
		B_STAT_MODIFICATION_TIME | B_STAT_ACCESS_TIME);

	RETURN_AND_SET_ERRNO(status);
}

