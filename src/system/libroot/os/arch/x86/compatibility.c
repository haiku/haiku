/*
 * Copyright 2005-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

// TODO: temporary measurement to fix the build with GCC 4 - in the end, these
//	private syscalls shouldn't be used anywhere in the Haiku tree, at the very
//	least they should only be used when compiling for GCC 2.95.3, though.
#if __GNUC__ < 30

// This file includes some known R5 syscalls
// ToDo: maybe they should indeed do something...

#include <SupportDefs.h>
#include <fs_info.h>

#include <syscalls.h>

#include <errno.h>
#include <sys/resource.h>
#include <sys/stat.h>


int _kset_mon_limit_(int num);
int _kset_fd_limit_(int num);
int _klock_node_(int fd);
int _kget_cpu_state_(int cpuNum);
int _kset_cpu_state_(int cpuNum, int state);
int _kstatfs_(dev_t device, void *whatever, int fd, const char *path, fs_info *info);


int
_kset_mon_limit_(int num)
{
	struct rlimit rl;
	if (num < 1)
		return EINVAL;
	rl.rlim_cur = num;
	if (setrlimit(RLIMIT_NOVMON, &rl) < 0)
		return errno;
	return B_OK;
}


int
_kset_fd_limit_(int num)
{
	struct rlimit rl;
	if (num < 1)
		return EINVAL;
	rl.rlim_cur = num;
	if (setrlimit(RLIMIT_NOFILE, &rl) < 0)
		return errno;
	return B_OK;
}


int
_klock_node_(int fd)
{
	return B_ERROR;
}


int
_kget_cpu_state_(int cpuNum)
{
	return _kern_cpu_enabled(cpuNum);
}


int
_kset_cpu_state_(int cpuNum, int state)
{
	return _kern_set_cpu_enabled(cpuNum, state != 0);
}


int
_kstatfs_(dev_t device, void *whatever, int fd, const char *path, fs_info *info)
{
	if (device < 0) {
		if (fd >= 0) {
			struct stat stat;
			if (fstat(fd, &stat) < 0)
				return -1;

			device = stat.st_dev;
		} else if (path != NULL)
			device = dev_for_path(path);
	}
	if (device < 0)
		return B_ERROR;

	return fs_stat_dev(device, info);
}


#endif	// __GNUC__ < 3
