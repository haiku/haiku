/*
 * Copyright 2018, Haiku Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_COMPAT_THREAD_DEFS_H
#define _KERNEL_COMPAT_THREAD_DEFS_H


#include <thread_defs.h>


#define compat_ptr_t uint32
struct compat_thread_creation_attributes {
	compat_ptr_t	entry;
	compat_ptr_t	name;
	int32		priority;
	compat_ptr_t	args1;
	compat_ptr_t	args2;
	compat_ptr_t	stack_address;
	uint32_t	stack_size;
	uint32_t	guard_size;
	compat_ptr_t	pthread;
	uint32		flags;
} _PACKED;


static_assert(sizeof(struct compat_thread_creation_attributes) == 40,
	"size of compat_thread_creation_attributes mismatch");


inline status_t
copy_ref_var_from_user(BKernel::ThreadCreationAttributes* userAttrs,
	BKernel::ThreadCreationAttributes &attrs)
{
	if (!IS_USER_ADDRESS(userAttrs))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		struct compat_thread_creation_attributes compatAttrs;
		if (user_memcpy(&compatAttrs, userAttrs, sizeof(compatAttrs)) < B_OK)
			return B_BAD_ADDRESS;
		attrs.entry = (int32 (*)(void*, void*))(addr_t)compatAttrs.entry;
		attrs.name = (const char*)(addr_t)compatAttrs.name;
		attrs.priority = compatAttrs.priority;
		attrs.args1 = (void*)(addr_t)compatAttrs.args1;
		attrs.args2 = (void*)(addr_t)compatAttrs.args2;
		attrs.stack_address = (void*)(addr_t)compatAttrs.stack_address;
		attrs.stack_size = compatAttrs.stack_size;
		attrs.guard_size = compatAttrs.guard_size;
		attrs.pthread = (pthread_t)(addr_t)compatAttrs.pthread;
		attrs.flags = compatAttrs.flags;
	} else if (user_memcpy(&attrs, userAttrs, sizeof(attrs)) < B_OK) {
		return B_BAD_ADDRESS;
	}
	return B_OK;
}


#endif // _KERNEL_COMPAT_THREAD_DEFS_H
