/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <fs_info.h>

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include "syscalls.h"

// ToDo: implement these for real - the VFS functions are still missing!

#define RETURN_AND_SET_ERRNO(status) \
	{ \
		if (status < 0) { \
			errno = status; \
			return -1; \
		} \
		return status; \
	}


dev_t
dev_for_path(const char *path)
{
	struct stat stat;
	int status = _kern_read_path_stat(path, false, &stat, sizeof(struct stat));
	if (status == B_OK)
		return stat.st_dev;

	RETURN_AND_SET_ERRNO(status);
}


dev_t
next_dev(int32 *pos)
{
	status_t status = B_ERROR;

	RETURN_AND_SET_ERRNO(status)
}


int
fs_stat_dev(dev_t dev, fs_info *info)
{
	status_t status = B_ERROR;

	RETURN_AND_SET_ERRNO(status)
}


