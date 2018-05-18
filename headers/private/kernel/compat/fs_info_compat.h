/*
 * Copyright 2018, Haiku Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_COMPAT_FS_INFO_H
#define _KERNEL_COMPAT_FS_INFO_H


#include <fs_info.h>


typedef struct compat_fs_info {
	dev_t	dev;								/* volume dev_t */
	ino_t	root;								/* root ino_t */
	uint32	flags;								/* flags (see above) */
	off_t	block_size;							/* fundamental block size */
	off_t	io_size;							/* optimal i/o size */
	off_t	total_blocks;						/* total number of blocks */
	off_t	free_blocks;						/* number of free blocks */
	off_t	total_nodes;						/* total number of nodes */
	off_t	free_nodes;							/* number of free nodes */
	char	device_name[128];					/* device holding fs */
	char	volume_name[B_FILE_NAME_LENGTH];	/* volume name */
	char	fsh_name[B_OS_NAME_LENGTH];			/* name of fs handler */
} _PACKED compat_fs_info;


static_assert(sizeof(compat_fs_info) == 480,
	"size of compat_fs_info mismatch");


inline status_t
copy_ref_var_to_user(fs_info &info, fs_info* userInfo)
{
	if (!IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		compat_fs_info compat_info;
		compat_info.dev = info.dev;
		compat_info.root = info.root;
		compat_info.flags = info.flags;
		compat_info.block_size = info.block_size;
		compat_info.io_size = info.io_size;
		compat_info.total_blocks = info.total_blocks;
		compat_info.free_blocks = info.free_blocks;
		compat_info.total_nodes = info.total_nodes;
		compat_info.free_nodes = info.free_nodes;
		strlcpy(compat_info.device_name, info.device_name, 128);
		strlcpy(compat_info.volume_name, info.volume_name, B_FILE_NAME_LENGTH);
		strlcpy(compat_info.fsh_name, info.fsh_name, B_OS_NAME_LENGTH);
		if (user_memcpy(userInfo, &compat_info, sizeof(compat_info)) < B_OK)
			return B_BAD_ADDRESS;
	} else {
		if (user_memcpy(userInfo, &info, sizeof(info)) < B_OK)
			return B_BAD_ADDRESS;
	}
	return B_OK;
}


inline status_t
copy_ref_var_from_user(fs_info* userInfo, fs_info &info)
{
	if (!IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		compat_fs_info compat_info;
		if (user_memcpy(&compat_info, userInfo, sizeof(compat_info)) < B_OK)
			return B_BAD_ADDRESS;
		info.dev = compat_info.dev;
		info.root = compat_info.root;
		info.flags = compat_info.flags;
		info.block_size = compat_info.block_size;
		info.io_size = compat_info.io_size;
		info.total_blocks = compat_info.total_blocks;
		info.free_blocks = compat_info.free_blocks;
		info.total_nodes = compat_info.total_nodes;
		info.free_nodes = compat_info.free_nodes;
		strlcpy(info.device_name, compat_info.device_name, 128);
		strlcpy(info.volume_name, compat_info.volume_name, B_FILE_NAME_LENGTH);
		strlcpy(info.fsh_name, compat_info.fsh_name, B_OS_NAME_LENGTH);
	} else if (user_memcpy(&info, userInfo, sizeof(info)) < B_OK) {
		return B_BAD_ADDRESS;
	}
	return B_OK;
}


#endif // _KERNEL_COMPAT_FS_INFO_H
