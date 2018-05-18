/*
 * Copyright 2018, Haiku Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_COMPAT_FCNTL_H
#define _KERNEL_COMPAT_FCNTL_H


#include <fcntl.h>


struct compat_flock {
	short	l_type;
	short	l_whence;
	off_t	l_start;
	off_t	l_len;
	pid_t	l_pid;
} _PACKED;


static_assert(sizeof(struct compat_flock) == 24,
	"size of compat_flock mismatch");


inline status_t
copy_ref_var_from_user(struct flock* userFlock, struct flock &flock)
{
	if (!IS_USER_ADDRESS(userFlock))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		compat_flock compat_flock;
		if (user_memcpy(&compat_flock, userFlock, sizeof(compat_flock)) < B_OK)
			return B_BAD_ADDRESS;
		flock.l_type = compat_flock.l_type;
		flock.l_whence = compat_flock.l_whence;
		flock.l_start = compat_flock.l_start;
		flock.l_len = compat_flock.l_len;
		flock.l_pid = compat_flock.l_pid;
	} else {
		if (user_memcpy(&flock, userFlock, sizeof(struct flock)) < B_OK)
			return B_BAD_ADDRESS;
	}
	return B_OK;
}


inline status_t
copy_ref_var_to_user(struct flock &flock, struct flock* userFlock)
{
	if (!IS_USER_ADDRESS(userFlock))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		compat_flock compat_flock;
		compat_flock.l_type = flock.l_type;
		compat_flock.l_whence = flock.l_whence;
		compat_flock.l_start = flock.l_start;
		compat_flock.l_len = flock.l_len;
		compat_flock.l_pid = flock.l_pid;
		if (user_memcpy(userFlock, &compat_flock, sizeof(compat_flock)) < B_OK)
			return B_BAD_ADDRESS;
	} else {
		if (user_memcpy(userFlock, &flock, sizeof(flock)) < B_OK)
			return B_BAD_ADDRESS;
	}
	return B_OK;
}


#endif // _KERNEL_COMPAT_FCNTL_H
