/*
 * Copyright 2018, Haiku Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_COMPAT_FS_ATTR_H
#define _KERNEL_COMPAT_FS_ATTR_H


#include <fs_attr.h>


struct compat_attr_info {
	uint32	type;
	off_t	size;
} _PACKED;


static_assert(sizeof(compat_attr_info) == 12,
	"size of compat_attr_info mismatch");


inline status_t
copy_ref_var_to_user(attr_info &info, attr_info* userInfo)
{
	if (!IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		compat_attr_info compat_info;
		compat_info.type = info.type;
		compat_info.size = info.size;
		if (user_memcpy(userInfo, &compat_info, sizeof(compat_info)) < B_OK)
			return B_BAD_ADDRESS;
	} else {
		if (user_memcpy(userInfo, &info, sizeof(info)) < B_OK)
			return B_BAD_ADDRESS;
	}
	return B_OK;
}


#endif // _KERNEL_COMPAT_FS_ATTR_H
