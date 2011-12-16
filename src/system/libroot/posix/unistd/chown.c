/*
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <unistd.h>

#include <NodeMonitor.h>

#include <errno_private.h>
#include <syscalls.h>
#include <syscall_utils.h>


int
chown(const char *path, uid_t owner, gid_t group)
{
	struct stat stat;
	status_t status;

	stat.st_uid = owner;
	stat.st_gid = group;
	status = _kern_write_stat(-1, path, true, &stat, sizeof(struct stat),
		B_STAT_UID | B_STAT_GID);

	RETURN_AND_SET_ERRNO(status);
}


int
lchown(const char *path, uid_t owner, gid_t group)
{
	struct stat stat;
	status_t status;

	stat.st_uid = owner;
	stat.st_gid = group;
	status = _kern_write_stat(-1, path, false, &stat, sizeof(struct stat),
		B_STAT_UID | B_STAT_GID);

	RETURN_AND_SET_ERRNO(status);
}


int
fchown(int fd, uid_t owner, gid_t group)
{
	struct stat stat;
	status_t status;

	stat.st_uid = owner;
	stat.st_gid = group;
	status = _kern_write_stat(fd, NULL, false, &stat, sizeof(struct stat),
		B_STAT_UID | B_STAT_GID);

	RETURN_AND_SET_ERRNO(status);
}


int
fchownat(int fd, const char* path, uid_t owner, gid_t group, int flag)
{
	struct stat stat;
	status_t status;

	stat.st_uid = owner;
	stat.st_gid = group;
	status = _kern_write_stat(fd, path, (flag & AT_SYMLINK_NOFOLLOW) == 0, &stat,
		sizeof(struct stat), B_STAT_UID | B_STAT_GID);

	RETURN_AND_SET_ERRNO(status);
}
