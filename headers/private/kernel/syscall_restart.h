/*
 * Copyright 2008-2013, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_SYSCALL_RESTART_H
#define _KERNEL_SYSCALL_RESTART_H


#include <OS.h>

#include <thread.h>


/*! Helper function for syscalls with relative timeout.
	Converts the given relative timeout to an absolute timeout or retrieves
	the value from the syscall restart parameters, if the syscall has been
	restarted. A negative value means infinite timeout.
*/
static inline void
syscall_restart_handle_timeout_pre(bigtime_t& timeout)
{
	// If restarted, get the timeout from the restart parameters. Otherwise
	// convert relative timeout to an absolute one.
	Thread* thread = thread_get_current_thread();
	if ((thread->flags & THREAD_FLAGS_SYSCALL_RESTARTED) != 0)
		timeout = *(bigtime_t*)thread->syscall_restart.parameters;
	else if (timeout >= 0) {
		timeout += system_time();
		// deal with overflow
		if (timeout < 0)
			timeout = B_INFINITE_TIMEOUT;
	}
}


/*! Helper function for syscalls with flags + timeout.
	If necessary converts the given timeout to an absolute timeout or retrieves
	the value from the syscall restart parameters, if the syscall has been
	restarted.
*/
static inline void
syscall_restart_handle_timeout_pre(uint32& flags, bigtime_t& timeout)
{
	// If restarted, get the timeout from the restart parameters. Otherwise
	// convert relative timeout to an absolute one. Note that we preserve
	// relative 0 us timeouts, so that the syscall can still decide whether to
	// return B_WOULD_BLOCK instead of B_TIMED_OUT.
	Thread* thread = thread_get_current_thread();
	if ((thread->flags & THREAD_FLAGS_SYSCALL_RESTARTED) != 0) {
		timeout = *(bigtime_t*)thread->syscall_restart.parameters;
		if (timeout > 0 && (flags & B_RELATIVE_TIMEOUT) != 0)
			flags = (flags & ~B_RELATIVE_TIMEOUT) | B_ABSOLUTE_TIMEOUT;
	} else if ((flags & B_RELATIVE_TIMEOUT)) {
		if (timeout > 0 && timeout < B_INFINITE_TIMEOUT) {
			timeout += system_time();
			// deal with overflow
			if (timeout < 0)
				timeout = B_INFINITE_TIMEOUT;

			flags = (flags & ~B_RELATIVE_TIMEOUT) | B_ABSOLUTE_TIMEOUT;
		}
	}
}


static inline status_t
syscall_restart_handle_timeout_post(status_t error, bigtime_t timeout)
{
	if (error == B_INTERRUPTED) {
		// interrupted -- store timeout and set flag for syscall restart
		Thread* thread = thread_get_current_thread();
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
		Thread* thread = thread_get_current_thread();
		atomic_or(&thread->flags, THREAD_FLAGS_RESTART_SYSCALL);
	}

	return error;
}


static inline bool
syscall_restart_is_restarted()
{
	Thread* thread = thread_get_current_thread();

	return (thread->flags & THREAD_FLAGS_SYSCALL) != 0
		&& (thread->flags & THREAD_FLAGS_SYSCALL_RESTARTED) != 0;
}


/*!	Returns whether or not a function has been called via a syscall. The flag
	to determine this is currently only used where actually needed, such as
	ioctl().

	TODO: this function is actually needed as part of the public API for ioctl()
*/
static inline bool
is_called_via_syscall(void)
{
	Thread* thread = thread_get_current_thread();
	return (thread->flags & THREAD_FLAGS_SYSCALL) != 0;
}


struct SyscallFlagUnsetter {
	SyscallFlagUnsetter()
	{
		fThread = thread_get_current_thread();
		fWasSyscall = (atomic_and(&fThread->flags, ~THREAD_FLAGS_SYSCALL)
			& THREAD_FLAGS_SYSCALL) != 0;
	}

	~SyscallFlagUnsetter()
	{
		if (fWasSyscall)
			atomic_or(&fThread->flags, THREAD_FLAGS_SYSCALL);
	}

private:
	Thread*	fThread;
	bool	fWasSyscall;
};


template<typename Type>
struct SyscallRestartWrapper {
	SyscallRestartWrapper(Type initialValue = 0)
		: fResult(initialValue)
	{
		fThread = thread_get_current_thread();
		atomic_or(&fThread->flags, THREAD_FLAGS_SYSCALL);
	}

	~SyscallRestartWrapper()
	{
		if (fResult == B_INTERRUPTED) {
			// interrupted -- set flag for syscall restart
			atomic_or(&fThread->flags, THREAD_FLAGS_RESTART_SYSCALL);
		}

		atomic_and(&fThread->flags, ~THREAD_FLAGS_SYSCALL);
	}

	SyscallRestartWrapper<Type>& operator=(const Type& other)
	{
		fResult = other;
		return *this;
	}

	operator Type() const	{ return fResult; }

private:
	Type	fResult;
	Thread*	fThread;
};


#endif	// _KERNEL_SYSCALL_RESTART_H
