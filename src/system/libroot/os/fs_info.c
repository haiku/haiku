/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include <fs_info.h>

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include <errno_private.h>
#include <syscalls.h>
#include <syscall_utils.h>


dev_t
dev_for_path(const char *path)
{
	struct stat stat;
	int status = _kern_read_stat(-1, path, true, &stat, sizeof(struct stat));
	if (status == B_OK)
		return stat.st_dev;

	RETURN_AND_SET_ERRNO(status);
}


dev_t
next_dev(int32 *_cookie)
{
	return _kern_next_device(_cookie);
		// For some reason, this one returns its error code directly
}


int
fs_stat_dev(dev_t device, fs_info *info)
{
	status_t status = _kern_read_fs_info(device, info);

	RETURN_AND_SET_ERRNO(status);
}


