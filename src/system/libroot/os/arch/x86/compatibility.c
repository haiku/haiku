/*
 * Copyright 2005-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

//!	This file includes some known R5 syscalls

#include <errno.h>
#include <sys/resource.h>
#include <sys/stat.h>

#include <SupportDefs.h>
#include <errno_private.h>
#include <fs_info.h>
#include <fs_volume.h>

#include <syscalls.h>


int _kset_mon_limit_(int num);
int _kset_fd_limit_(int num);
int _klock_node_(int fd);
int _kunlock_node_(int fd);
int _kget_cpu_state_(int cpuNum);
int _kset_cpu_state_(int cpuNum, int state);
int _kstatfs_(dev_t device, void *whatever, int fd, const char *path, fs_info *info);
int mount(const char *filesystem, const char *where, const char *device, ulong flags,
	void *parms, int len);
int unmount(const char *path);


int
_kset_mon_limit_(int num)
{
	struct rlimit rl;
	if (num < 1)
		return EINVAL;
	rl.rlim_cur = num;
	rl.rlim_max = RLIM_SAVED_MAX;
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
	rl.rlim_max = RLIM_SAVED_MAX;
	if (setrlimit(RLIMIT_NOFILE, &rl) < 0)
		return errno;
	return B_OK;
}


int
_klock_node_(int fd)
{
	return _kern_lock_node(fd);
}


int
_kunlock_node_(int fd)
{
	return _kern_unlock_node(fd);
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


int
mount(const char *filesystem, const char *where, const char *device, ulong flags,
	void *parms, int len)
{
	status_t err;
	uint32 mountFlags = 0;

	if (flags & 1)
		mountFlags |= B_MOUNT_READ_ONLY;

	// we don't support passing (binary) parameters
	if (parms != NULL || len != 0 || flags & ~1) {
		__set_errno(B_BAD_VALUE);
		return -1;
	}

	err = fs_mount_volume(where, device, filesystem, mountFlags, NULL);
	if (err < B_OK) {
		__set_errno(err);
		return -1;
	}
	return 0;
}


int
unmount(const char *path)
{
	status_t err;

	err = fs_unmount_volume(path, 0);
	if (err < B_OK) {
		__set_errno(err);
		return -1;
	}
	return 0;
}

