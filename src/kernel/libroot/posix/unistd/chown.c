/* 
** Copyright 2002-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <fs_interface.h>

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
	status = _kern_write_path_stat(path, true, &stat, sizeof(struct stat),
		FS_WRITE_STAT_UID | FS_WRITE_STAT_GID);

	RETURN_AND_SET_ERRNO(status);
}


int
lchown(const char *path, uid_t owner, gid_t group)
{
	struct stat stat;
	status_t status;

	stat.st_uid = owner;
	stat.st_gid = group;
	status = _kern_write_path_stat(path, false, &stat, sizeof(struct stat),
		FS_WRITE_STAT_UID | FS_WRITE_STAT_GID);

	RETURN_AND_SET_ERRNO(status);
}


int
fchown(int fd, uid_t owner, gid_t group)
{
	struct stat stat;
	status_t status;

	stat.st_uid = owner;
	stat.st_gid = group;
	status = _kern_write_stat(fd, &stat, sizeof(struct stat),
		FS_WRITE_STAT_UID | FS_WRITE_STAT_GID);

	RETURN_AND_SET_ERRNO(status);
}

