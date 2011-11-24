/*
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <sys/stat.h>

#include <NodeMonitor.h>

#include <errno_private.h>
#include <syscalls.h>
#include <syscall_utils.h>


int
chmod(const char *path, mode_t mode)
{
	struct stat stat;
	status_t status;

	stat.st_mode = mode;
	status = _kern_write_stat(-1, path, true, &stat, sizeof(struct stat),
		B_STAT_MODE);

	RETURN_AND_SET_ERRNO(status);
}


int
fchmod(int fd, mode_t mode)
{
	struct stat stat;
	status_t status;

	stat.st_mode = mode;
	status = _kern_write_stat(fd, NULL, false, &stat, sizeof(struct stat),
		B_STAT_MODE);

	RETURN_AND_SET_ERRNO(status);
}


int
fchmodat(int fd, const char* path, mode_t mode, int flag)
{
	struct stat stat;
	status_t status;

	stat.st_mode = mode;
	status = _kern_write_stat(fd, path, (flag & AT_SYMLINK_NOFOLLOW) == 0, &stat,
		sizeof(struct stat), B_STAT_MODE);

	RETURN_AND_SET_ERRNO(status);
}
