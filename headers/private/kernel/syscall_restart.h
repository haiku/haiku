/*
 * Copyright 2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_SYSCALL_RESTART_H
#define _KERNEL_SYSCALL_RESTART_H

#include <OS.h>

#include <thread.h>


static inline void
syscall_restart_handle_timeout_pre(bigtime_t& timeout)
{
	// If restarted, get the timeout from the restart parameters. Otherwise
	// convert relative timeout to an absolute one.
	struct thread* thread = thread_get_current_thread();
	if ((thread->flags & THREAD_FLAGS_SYSCALL_RESTARTED) != 0)
		timeout = *(bigtime_t*)thread->syscall_restart.parameters;
	else if (timeout >= 0) {
		timeout += system_time();
		if (timeout < 0)
			timeout = B_INFINITE_TIMEOUT;
	}
}


static inline void
syscall_restart_handle_timeout_pre(uint32& flags, bigtime_t& timeout)
{
	// If restarted, get the timeout from the restart parameters. Otherwise
	// convert relative timeout to an absolute one.
	struct thread* thread = thread_get_current_thread();
	if ((thread->flags & THREAD_FLAGS_SYSCALL_RESTARTED) != 0)
		timeout = *(bigtime_t*)thread->syscall_restart.parameters;
	else if ((flags & B_RELATIVE_TIMEOUT) != 0) {
		timeout += system_time();
		if (timeout < 0)
			timeout = B_INFINITE_TIMEOUT;
	}

	// any timeout is absolute at this point
	if ((flags & B_RELATIVE_TIMEOUT) != 0)
		flags = (flags & ~B_RELATIVE_TIMEOUT) | B_ABSOLUTE_TIMEOUT;
}


static inline status_t
syscall_restart_handle_timeout_post(status_t error, bigtime_t timeout)
{
	if (error == B_INTERRUPTED) {
		// interrupted -- store timeout and set flag for syscall restart
		struct thread* thread = thread_get_current_thread();
		*(bigtime_t*)thread->syscall_restart.parameters = timeout;
		atomic_or(&thread->flags, THREAD_FLAGS_RESTART_SYSCALL);
	}

	return error;
}


static inline status_t
syscall_restart_handle_post(status_t error)
{
	if (error == B_INTERRUPTED) {
		// interrupted -- set flag for syscall restart
		struct thread* thread = thread_get_current_thread();
		atomic_or(&thread->flags, THREAD_FLAGS_RESTART_SYSCALL);
	}

	return error;
}


static inline bool
syscall_restart_ioctl_is_restarted()
{
	struct thread* thread = thread_get_current_thread();

	return (thread->flags & THREAD_FLAGS_IOCTL_SYSCALL) != 0
		&& (thread->flags & THREAD_FLAGS_SYSCALL_RESTARTED) != 0;
}


static inline status_t
syscall_restart_ioctl_handle_post(status_t error)
{
	if (error == B_INTERRUPTED) {
		// interrupted -- set flag for syscall restart
		struct thread* thread = thread_get_current_thread();
		if ((thread->flags & THREAD_FLAGS_IOCTL_SYSCALL) != 0)
			atomic_or(&thread->flags, THREAD_FLAGS_RESTART_SYSCALL);
	}

	return error;
}


#endif	// _KERNEL_SYSCALL_RESTART_H
