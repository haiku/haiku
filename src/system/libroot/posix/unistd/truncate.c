/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <fs_interface.h>
#include <NodeMonitor.h>

#include <unistd.h>
#include <syscalls.h>
#include <errno.h>

#include <errno_private.h>


#define RETURN_AND_SET_ERRNO(err) \
	if (err < 0) { \
		__set_errno(err); \
		return -1; \
	} \
	return err;


int
truncate(const char *path, off_t newSize)
{
	struct stat stat;
	status_t status;

	stat.st_size = newSize;
	status = _kern_write_stat(-1, path, true, &stat, sizeof(struct stat),
		B_STAT_SIZE);

	RETURN_AND_SET_ERRNO(status);
}


int
ftruncate(int fd, off_t newSize)
{
	struct stat stat;
	status_t status;

	stat.st_size = newSize;
	status = _kern_write_stat(fd, NULL, false, &stat, sizeof(struct stat),
		B_STAT_SIZE);

	RETURN_AND_SET_ERRNO(status);
}

