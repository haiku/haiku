/*
 * Copyright 2018, Haiku Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_COMPAT_SIGNAL_H
#define _KERNEL_COMPAT_SIGNAL_H


struct compat_sigaction {
	union {
		uint32		sa_handler;
		uint32		sa_sigaction;
	};
	sigset_t				sa_mask;
	int						sa_flags;
	uint32					sa_userdata;	/* will be passed to the signal
											   handler, BeOS extension */
} _PACKED;


inline status_t
copy_ref_var_from_user(struct sigaction* userAction, struct sigaction &action)
{
	if (!IS_USER_ADDRESS(userAction))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		struct compat_sigaction compat_action;
		if (user_memcpy(&compat_action, userAction, sizeof(compat_sigaction))
				< B_OK) {
			return B_BAD_ADDRESS;
		}
		action.sa_handler = (__sighandler_t)(addr_t)compat_action.sa_handler;
		action.sa_mask = compat_action.sa_mask;
		action.sa_flags = compat_action.sa_flags;
		action.sa_userdata = (void*)(addr_t)compat_action.sa_userdata;
	} else if (user_memcpy(&action, userAction, sizeof(action)) < B_OK) {
		return B_BAD_ADDRESS;
	}
	return B_OK;
}


inline status_t
copy_ref_var_to_user(struct sigaction &action, struct sigaction* userAction)
{
	if (!IS_USER_ADDRESS(userAction))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		struct compat_sigaction compat_action;
		compat_action.sa_handler = (addr_t)action.sa_handler;
		compat_action.sa_mask = action.sa_mask;
		compat_action.sa_flags = action.sa_flags;
		compat_action.sa_userdata = (addr_t)action.sa_userdata;
		if (user_memcpy(userAction, &compat_action, sizeof(compat_action))
				< B_OK) {
			return B_BAD_ADDRESS;
		}
	} else if (user_memcpy(userAction, &action, sizeof(action)) < B_OK) {
		return B_BAD_ADDRESS;
	}
	return B_OK;
}


#endif // _KERNEL_COMPAT_SIGNAL_H
