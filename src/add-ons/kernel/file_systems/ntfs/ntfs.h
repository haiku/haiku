/*
 * Copyright 2021 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Augustin Cavalier, <waddlesplash>
 */
#ifndef NTFS_H
#define NTFS_H


#include <StorageDefs.h>
#include <lock.h>
#include <fs_volume.h>

extern "C" {
#include "libntfs/types.h"
#include "libntfs/volume.h"

#include "lowntfs.h"
}


extern fs_volume_ops gNtfsVolumeOps;
extern fs_vnode_ops gNtfsVnodeOps;


struct volume {
	volume()
	{
		mutex_init(&lock, "NTFS volume lock");
		lowntfs.abs_mnt_point = NULL;
	}
	~volume()
	{
		mutex_destroy(&lock);
		free(lowntfs.abs_mnt_point);
	}

	mutex				lock;
	uint32				fs_info_flags;

	ntfs_volume*		ntfs;
	lowntfs_context		lowntfs;
};

typedef struct vnode {
	~vnode()
	{
		free(name);
	}

	u64			inode;
	u64			parent_inode;

	mode_t		mode;
	uid_t		uid;
	gid_t		gid;
	s64			size;

	int			lowntfs_close_state = 0;
	u64			lowntfs_ghost = 0;

	char*		name = NULL;
	void*		file_cache = NULL;
} vnode;


typedef struct file_cookie {
	int			open_mode;

	s64			last_size = 0;
	bigtime_t	last_notification = 0;
#define INODE_NOTIFICATION_INTERVAL	1000000LL
} file_cookie;

typedef struct directory_cookie {
	typedef struct entry {
		entry* next;

		u64 inode;
		u32 name_length;
		char name[1]; /* variable length! */
	} entry;
	entry* first, *current;
} directory_cookie;


#endif	// NTFS_H
