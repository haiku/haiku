/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de. All rights reserved.
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <sys/stat.h>

#include <compat/sys/stat.h>

#include <errno_private.h>
#include <syscalls.h>
#include <symbol_versioning.h>
#include <syscall_utils.h>


// prototypes for the compiler
int _stat_current(const char* path, struct stat* stat);
int _fstat_current(int fd, struct stat* stat);
int _lstat_current(const char* path, struct stat* stat);
int _stat_beos(const char* path, struct stat_beos* beosStat);
int _fstat_beos(int fd, struct stat_beos* beosStat);
int _lstat_beos(const char* path, struct stat_beos* beosStat);


int
_stat_current(const char* path, struct stat* stat)
{
	int status = _kern_read_stat(-1, path, true, stat, sizeof(struct stat));

	RETURN_AND_SET_ERRNO(status);
}


int
_fstat_current(int fd, struct stat* stat)
{
	int status = _kern_read_stat(fd, NULL, false, stat, sizeof(struct stat));

	RETURN_AND_SET_ERRNO(status);
}


int
_lstat_current(const char* path, struct stat* stat)
{
	int status = _kern_read_stat(-1, path, false, stat, sizeof(struct stat));

	RETURN_AND_SET_ERRNO(status);
}


int
fstatat(int fd, const char *path, struct stat *st, int flag)
{
	int status = _kern_read_stat(fd, path, (flag & AT_SYMLINK_NOFOLLOW) == 0,
		st, sizeof(struct stat));

	RETURN_AND_SET_ERRNO(status);
}


// #pragma mark - BeOS compatibility


void
convert_to_stat_beos(const struct stat* stat, struct stat_beos* beosStat)
{
	if (stat == NULL || beosStat == NULL)
		return;

	beosStat->st_dev = stat->st_dev;
	beosStat->st_ino = stat->st_ino;
	beosStat->st_mode = stat->st_mode;
	beosStat->st_nlink = stat->st_nlink;
	beosStat->st_uid = stat->st_uid;
	beosStat->st_gid = stat->st_gid;
	beosStat->st_size = stat->st_size;
	beosStat->st_rdev = stat->st_rdev;
	beosStat->st_blksize = stat->st_blksize;
	beosStat->st_atime = stat->st_atime;
	beosStat->st_mtime = stat->st_mtime;
	beosStat->st_ctime = stat->st_ctime;
	beosStat->st_crtime = stat->st_crtime;
}


void
convert_from_stat_beos(const struct stat_beos* beosStat, struct stat* stat)
{
	if (stat == NULL || beosStat == NULL)
		return;

	stat->st_dev = beosStat->st_dev;
	stat->st_ino = beosStat->st_ino;
	stat->st_mode = beosStat->st_mode;
	stat->st_nlink = beosStat->st_nlink;
	stat->st_uid = beosStat->st_uid;
	stat->st_gid = beosStat->st_gid;
	stat->st_size = beosStat->st_size;
	stat->st_rdev = beosStat->st_rdev;
	stat->st_blksize = beosStat->st_blksize;
	stat->st_atim.tv_sec = beosStat->st_atime;
	stat->st_atim.tv_nsec = 0;
	stat->st_mtim.tv_sec = beosStat->st_mtime;
	stat->st_mtim.tv_nsec = 0;
	stat->st_ctim.tv_sec = beosStat->st_ctime;
	stat->st_ctim.tv_nsec = 0;
	stat->st_crtim.tv_sec = beosStat->st_crtime;
	stat->st_crtim.tv_nsec = 0;
	stat->st_type = 0;
	stat->st_blocks = 0;
}


int
_stat_beos(const char* path, struct stat_beos* beosStat)
{
	struct stat stat;
	if (_stat_current(path, &stat) != 0)
		return -1;

	convert_to_stat_beos(&stat, beosStat);

	return 0;
}


int
_fstat_beos(int fd, struct stat_beos* beosStat)
{
	struct stat stat;
	if (_fstat_current(fd, &stat) != 0)
		return -1;

	convert_to_stat_beos(&stat, beosStat);

	return 0;
}


int
_lstat_beos(const char* path, struct stat_beos* beosStat)
{
	struct stat stat;
	if (_lstat_current(path, &stat) != 0)
		return -1;

	convert_to_stat_beos(&stat, beosStat);

	return 0;
}


DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("_stat_beos", "stat@", "BASE");
DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("_fstat_beos", "fstat@", "BASE");
DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("_lstat_beos", "lstat@", "BASE");

DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("_stat_current", "stat@@", "1_ALPHA1");
DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("_fstat_current", "fstat@@", "1_ALPHA1");
DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("_lstat_current", "lstat@@", "1_ALPHA1");
