/*
 * Copyright 2018, Haiku Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_COMPAT_KSIGNAL_H
#define _KERNEL_COMPAT_KSIGNAL_H


#include <compat/signal_compat.h>
#include <compat/arch/x86/signal_compat.h>


typedef struct compat_vregs compat_mcontext_t;

typedef struct __compat_ucontext_t {
	uint32					uc_link;
	sigset_t				uc_sigmask;
	compat_stack_t			uc_stack;
	compat_mcontext_t		uc_mcontext;
} _PACKED compat_ucontext_t;

union compat_sigval {
	int		sival_int;
	uint32	sival_ptr;
};


typedef struct __compat_siginfo_t {
	int				si_signo;	/* signal number */
	int				si_code;	/* signal code */
	int				si_errno;	/* if non zero, an error number associated with
								   this signal */
	pid_t			si_pid;		/* sending process ID */
	uid_t			si_uid;		/* real user ID of sending process */
	uint32			si_addr;	/* address of faulting instruction */
	int				si_status;	/* exit value or signal */
	uint32			si_band;	/* band event for SIGPOLL */
	union compat_sigval	si_value;	/* signal value */
} compat_siginfo_t;


struct compat_signal_frame_data {
	compat_siginfo_t	info;
	compat_ucontext_t	context;
	uint32		user_data;
	uint32		handler;
	bool		siginfo_handler;
	char		_pad[3];
	int32		thread_flags;
	uint64		syscall_restart_return_value;
	uint8		syscall_restart_parameters[SYSCALL_RESTART_PARAMETER_SIZE];
	uint32		commpage_address;
} _PACKED;


static_assert(sizeof(compat_stack_t) == 12,
	"size of compat_stack_t mismatch");
static_assert(sizeof(compat_mcontext_t) == 560,
	"size of compat_mcontext_t mismatch");
static_assert(sizeof(compat_ucontext_t) == 584,
	"size of compat_ucontext_t mismatch");
static_assert(sizeof(compat_siginfo_t) == 36,
	"size of compat_siginfo_t mismatch");
static_assert(sizeof(struct compat_signal_frame_data) == 680,
	"size of compat_signal_frame_data mismatch");


inline status_t
copy_ref_var_from_user(union sigval* userSigval, union sigval &sigval)
{
	if (!IS_USER_ADDRESS(userSigval))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		compat_sigval compat_sigval;
		if (user_memcpy(&compat_sigval, userSigval, sizeof(compat_sigval))
				< B_OK) {
			return B_BAD_ADDRESS;
		}
		sigval.sival_ptr = (void*)(addr_t)compat_sigval.sival_ptr;
	} else if (user_memcpy(&sigval, userSigval, sizeof(union sigval)) < B_OK) {
		return B_BAD_ADDRESS;
	}
	return B_OK;
}


inline status_t
copy_ref_var_to_user(siginfo_t &info, siginfo_t* userInfo)
{
	if (!IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		compat_siginfo_t compat_info;
		compat_info.si_signo = info.si_signo;
		compat_info.si_code = info.si_code;
		compat_info.si_errno = info.si_errno;
		compat_info.si_pid = info.si_pid;
		compat_info.si_uid = info.si_uid;
		compat_info.si_addr = (addr_t)info.si_addr;
		compat_info.si_status = info.si_status;
		compat_info.si_band = info.si_band;
		compat_info.si_value.sival_ptr = (addr_t)info.si_value.sival_ptr;
		if (user_memcpy(userInfo, &compat_info, sizeof(compat_info))
				< B_OK) {
			return B_BAD_ADDRESS;
		}
	} else if (user_memcpy(userInfo, &info, sizeof(info)) < B_OK) {
		return B_BAD_ADDRESS;
	}
	return B_OK;
}


inline status_t
copy_ref_var_from_user(stack_t* userStack, stack_t &stack)
{
	if (!IS_USER_ADDRESS(userStack))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		compat_stack_t compatStack;
		if (user_memcpy(&compatStack, userStack, sizeof(compatStack))
				< B_OK) {
			return B_BAD_ADDRESS;
		}
		stack.ss_sp = (void*)(addr_t)compatStack.ss_sp;
		stack.ss_size = compatStack.ss_size;
		stack.ss_flags = compatStack.ss_flags;
	} else if (user_memcpy(&stack, userStack, sizeof(stack_t)) < B_OK) {
		return B_BAD_ADDRESS;
	}
	return B_OK;
}


inline status_t
copy_ref_var_to_user(stack_t &stack, stack_t* userStack)
{
	if (!IS_USER_ADDRESS(userStack))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		compat_stack_t compatStack;
		compatStack.ss_sp = (addr_t)stack.ss_sp;
		compatStack.ss_size = stack.ss_size;
		compatStack.ss_flags = stack.ss_flags;
		if (user_memcpy(userStack, &compatStack, sizeof(compatStack))
				< B_OK) {
			return B_BAD_ADDRESS;
		}
	} else if (user_memcpy(userStack, &stack, sizeof(stack)) < B_OK) {
		return B_BAD_ADDRESS;
	}
	return B_OK;
}


#endif // _KERNEL_COMPAT_KSIGNAL_H
