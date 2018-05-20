/*
 * Copyright 2018, Haiku Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_COMPAT_RESOURCE_H
#define _KERNEL_COMPAT_RESOURCE_H


#include <sys/resource.h>


typedef uint32 compat_rlim_t;
struct compat_rlimit {
	compat_rlim_t	rlim_cur;		/* current soft limit */
	compat_rlim_t	rlim_max;		/* hard limit */
};


inline status_t
copy_ref_var_to_user(struct rlimit &rl, struct rlimit* urlp)
{
	if (!IS_USER_ADDRESS(urlp))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		struct compat_rlimit compat_rl;
		compat_rl.rlim_cur = rl.rlim_cur;
		compat_rl.rlim_max = rl.rlim_max;
		if (user_memcpy(urlp, &compat_rl, sizeof(compat_rl)) < B_OK)
			return B_BAD_ADDRESS;
	} else if (user_memcpy(urlp, &rl, sizeof(rl)) < B_OK) {
		return B_BAD_ADDRESS;
	}
	return B_OK;
}


inline status_t
copy_ref_var_from_user(struct rlimit* urlp, struct rlimit &rl)
{
	if (!IS_USER_ADDRESS(urlp))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		struct compat_rlimit compat_rl;
		if (user_memcpy(&compat_rl, urlp, sizeof(compat_rl)) < B_OK)
			return B_BAD_ADDRESS;
		rl.rlim_cur = compat_rl.rlim_cur;
		rl.rlim_max = compat_rl.rlim_max;
	} else if (user_memcpy(&rl, urlp, sizeof(rl)) < B_OK) {
		return B_BAD_ADDRESS;
	}
	return B_OK;
}


#endif // _KERNEL_COMPAT_RESOURCE_H
