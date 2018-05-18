/*
 * Copyright 2018, Haiku Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_COMPAT_VFS_DEFS_H
#define _KERNEL_COMPAT_VFS_DEFS_H


#include <vfs_defs.h>


struct compat_fd_info {
	int		number;
	int32	open_mode;
	dev_t	device;
	ino_t	node;
} _PACKED;


static_assert(sizeof(compat_fd_info) == 20,
	"size of compat_fd_info mismatch");


inline status_t
copy_ref_var_to_user(attr_info &info, attr_info* userInfo, size_t size)
{
	if (!IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		if (size != sizeof(compat_fd_info))
			return B_BAD_VALUE;
		compat_fd_info compat_info;
		compat_info.number = info.number;
		compat_info.open_mode = info.open_mode;
		compat_info.device = info.device;
		compat_info.node = info.node;
		if (user_memcpy(userInfo, &compat_info, size) < B_OK)
			return B_BAD_ADDRESS;
	} else {
		if (size != sizeof(fd_info))
			return B_BAD_VALUE;
		if (user_memcpy(userInfo, &info, size) < B_OK)
			return B_BAD_ADDRESS;
	}
	return B_OK;
}


inline status_t
copy_ref_var_from_user(fd_info* userInfo, fd_info &info, size_t size)
{
	if (!IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		if (size != sizeof(compat_fd_info))
			return B_BAD_VALUE;
		compat_fd_info compat_info;
		if (user_memcpy(&compat_info, userInfo, size) < B_OK)
			return B_BAD_ADDRESS;
		info.number = compat_info.number;
		info.open_mode = compat_info.open_mode;
		info.device = compat_info.device;
		info.node = compat_info.node;
	} else {
		if (size != sizeof(fd_info))
			return B_BAD_VALUE;
		if (user_memcpy(&info, userInfo, size) < B_OK)
			return B_BAD_ADDRESS;
	}
	return B_OK;
}


#endif // _KERNEL_COMPAT_VFS_DEFS_H
