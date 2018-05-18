/*
 * Copyright 2018, Haiku Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_COMPAT_STAT_H
#define _KERNEL_COMPAT_STAT_H


#include <sys/stat.h>
#include <time_compat.h>


struct compat_stat {
	dev_t			st_dev;			/* device ID that this file resides on */
	ino_t			st_ino;			/* this file's serial inode ID */
	mode_t			st_mode;		/* file mode (rwx for user, group, etc) */
	nlink_t			st_nlink;		/* number of hard links to this file */
	uid_t			st_uid;			/* user ID of the owner of this file */
	gid_t			st_gid;			/* group ID of the owner of this file */
	off_t			st_size;		/* size in bytes of this file */
	dev_t			st_rdev;		/* device type (not used) */
	blksize_t		st_blksize;		/* preferred block size for I/O */
	struct compat_timespec	st_atim;		/* last access time */
	struct compat_timespec	st_mtim;		/* last modification time */
	struct compat_timespec	st_ctim;		/* last change time, not creation time */
	struct compat_timespec	st_crtim;		/* creation time */
	__haiku_uint32	st_type;		/* attribute/index type */
	blkcnt_t		st_blocks;		/* number of blocks allocated for object */
} _PACKED;


static_assert(sizeof(struct compat_stat) == 88,
	"size of struct compat_stat mismatch");


inline status_t
copy_ref_var_to_user(struct stat &stat, struct stat* userStat, size_t size)
{
	if (!IS_USER_ADDRESS(userStat))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		if (size > sizeof(compat_stat))
			return B_BAD_VALUE;
		struct compat_stat compat_stat;
		compat_stat.st_dev = stat.st_dev;
		compat_stat.st_ino = stat.st_ino;
		compat_stat.st_mode = stat.st_mode;
		compat_stat.st_nlink = stat.st_nlink;
		compat_stat.st_uid = stat.st_gid;
		compat_stat.st_size = stat.st_size;
		compat_stat.st_rdev = stat.st_rdev;
		compat_stat.st_blksize = stat.st_blksize;
		compat_stat.st_atim.tv_sec = stat.st_atim.tv_sec;
		compat_stat.st_atim.tv_nsec = stat.st_atim.tv_nsec;
		compat_stat.st_mtim.tv_sec = stat.st_mtim.tv_sec;
		compat_stat.st_mtim.tv_nsec = stat.st_mtim.tv_nsec;
		compat_stat.st_ctim.tv_sec = stat.st_ctim.tv_sec;
		compat_stat.st_ctim.tv_nsec = stat.st_ctim.tv_nsec;
		compat_stat.st_crtim.tv_sec = stat.st_crtim.tv_sec;
		compat_stat.st_crtim.tv_nsec = stat.st_crtim.tv_nsec;
		compat_stat.st_type = stat.st_type;
		compat_stat.st_blocks = stat.st_blocks;
		if (user_memcpy(userStat, &compat_stat, size) < B_OK)
			return B_BAD_ADDRESS;
	} else {
		if (size > sizeof(struct stat))
			return B_BAD_VALUE;

		if (user_memcpy(userStat, &stat, size) < B_OK)
			return B_BAD_ADDRESS;
	}
	return B_OK;
}


inline status_t
copy_ref_var_to_user(struct stat &stat, struct stat* userStat)
{
	if (!IS_USER_ADDRESS(userStat))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		struct compat_stat compat_stat;
		compat_stat.st_dev = stat.st_dev;
		compat_stat.st_ino = stat.st_ino;
		compat_stat.st_mode = stat.st_mode;
		compat_stat.st_nlink = stat.st_nlink;
		compat_stat.st_uid = stat.st_gid;
		compat_stat.st_size = stat.st_size;
		compat_stat.st_rdev = stat.st_rdev;
		compat_stat.st_blksize = stat.st_blksize;
		compat_stat.st_atim.tv_sec = stat.st_atim.tv_sec;
		compat_stat.st_atim.tv_nsec = stat.st_atim.tv_nsec;
		compat_stat.st_mtim.tv_sec = stat.st_mtim.tv_sec;
		compat_stat.st_mtim.tv_nsec = stat.st_mtim.tv_nsec;
		compat_stat.st_ctim.tv_sec = stat.st_ctim.tv_sec;
		compat_stat.st_ctim.tv_nsec = stat.st_ctim.tv_nsec;
		compat_stat.st_crtim.tv_sec = stat.st_crtim.tv_sec;
		compat_stat.st_crtim.tv_nsec = stat.st_crtim.tv_nsec;
		compat_stat.st_type = stat.st_type;
		compat_stat.st_blocks = stat.st_blocks;
		if (user_memcpy(userStat, &compat_stat, sizeof(compat_stat)) < B_OK)
			return B_BAD_ADDRESS;
	} else {
		if (user_memcpy(userStat, &stat, sizeof(stat)) < B_OK)
			return B_BAD_ADDRESS;
	}
	return B_OK;
}



#endif // _KERNEL_COMPAT_STAT_H
