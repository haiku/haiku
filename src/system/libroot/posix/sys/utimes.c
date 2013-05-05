/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2006-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <sys/time.h>

#include <errno.h>

#include <NodeMonitor.h>

#include <errno_private.h>
#include <syscalls.h>
#include <syscall_utils.h>


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


int
utimensat(int fd, const char *path, const struct timespec times[2], int flag)
{
	struct stat stat;
	status_t status;
	uint32 mask = 0;

	// Init the stat time fields to the current time, if at least one time is
	// supposed to be set to it.
	if (times == NULL || times[0].tv_nsec == UTIME_NOW
		|| times[1].tv_nsec == UTIME_NOW) {
		bigtime_t now = real_time_clock_usecs();
		stat.st_atim.tv_sec = stat.st_mtim.tv_sec = now / 1000000;
		stat.st_atim.tv_nsec = stat.st_mtim.tv_nsec = (now % 1000000) * 1000;
	}

	if (times != NULL) {
		// access time
		if (times[0].tv_nsec != UTIME_OMIT) {
			mask |= B_STAT_ACCESS_TIME;

			if (times[0].tv_nsec != UTIME_NOW) {
				if (times[0].tv_nsec < 0 || times[0].tv_nsec > 999999999)
					RETURN_AND_SET_ERRNO(EINVAL);
			}

			stat.st_atim = times[0];
		}

		// modified time
		if (times[1].tv_nsec != UTIME_OMIT) {
			mask |= B_STAT_MODIFICATION_TIME;

			if (times[1].tv_nsec != UTIME_NOW) {
				if (times[1].tv_nsec < 0 || times[1].tv_nsec > 999999999)
					RETURN_AND_SET_ERRNO(EINVAL);
			}

			stat.st_mtim = times[1];
		}
	} else
		mask |= B_STAT_ACCESS_TIME | B_STAT_MODIFICATION_TIME;

	// set the times -- as per spec we even need to do this, if both have
	// UTIME_OMIT set
	status = _kern_write_stat(fd, path, (flag & AT_SYMLINK_NOFOLLOW) == 0,
		&stat, sizeof(struct stat), mask);

	RETURN_AND_SET_ERRNO(status);
}


int
futimens(int fd, const struct timespec times[2])
{
	return utimensat(fd, NULL, times, 0);
}
