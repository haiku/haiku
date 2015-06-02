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


static int
common_chown(int fd, const char* path, bool followLinks, uid_t owner,
	gid_t group)
{
	struct stat stat;
	status_t status;

	int mask = 0;
	if (owner != (uid_t)-1) {
		stat.st_uid = owner;
		mask |= B_STAT_UID;
	}
	if (group != (gid_t)-1) {
		stat.st_gid = group;
		mask |= B_STAT_GID;
	}

	status = _kern_write_stat(fd, path, followLinks, &stat,
		sizeof(struct stat), mask);

	RETURN_AND_SET_ERRNO(status);
}


int
chown(const char *path, uid_t owner, gid_t group)
{
	return common_chown(-1, path, true, owner, group);
}


int
lchown(const char *path, uid_t owner, gid_t group)
{
	return common_chown(-1, path, false, owner, group);
}


int
fchown(int fd, uid_t owner, gid_t group)
{
	return common_chown(fd, NULL, false, owner, group);
}


int
fchownat(int fd, const char* path, uid_t owner, gid_t group, int flag)
{
	return common_chown(fd, path, (flag & AT_SYMLINK_NOFOLLOW) == 0, owner,
		group);
}
