/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <unistd.h>
#include <syscalls.h>
#include <errno.h>


#define RETURN_AND_SET_ERRNO(err) \
	if (err < 0) { \
		errno = err; \
		return -1; \
	} \
	return err;


int
chown(const char *path, uid_t owner, gid_t group)
{
	struct stat stat;
	status_t status;

	stat.st_uid = owner;
	stat.st_gid = group;
	status = sys_write_stat(-1, path, true, &stat, FS_WRITE_STAT_UID | FS_WRITE_STAT_GID);

	RETURN_AND_SET_ERRNO(status);
}


int
lchown(const char *path, uid_t owner, gid_t group)
{
	struct stat stat;
	status_t status;

	stat.st_uid = owner;
	stat.st_gid = group;
	status = sys_write_stat(-1, path, false, &stat, FS_WRITE_STAT_UID | FS_WRITE_STAT_GID);

	RETURN_AND_SET_ERRNO(status);
}


int
fchown(int fd, uid_t owner, gid_t group)
{
	struct stat stat;
	status_t status;

	stat.st_uid = owner;
	stat.st_gid = group;
	status = sys_write_stat(fd, NULL, true, &stat, FS_WRITE_STAT_UID | FS_WRITE_STAT_GID);

	RETURN_AND_SET_ERRNO(status);
}

