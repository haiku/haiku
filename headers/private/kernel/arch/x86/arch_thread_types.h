/*
 * Copyright 2002-2006, The Haiku Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_ARCH_x86_THREAD_TYPES_H
#define _KERNEL_ARCH_x86_THREAD_TYPES_H


#include <arch_cpu.h>


namespace BKernel {
    struct Thread;
}


#define _ALIGNED(bytes) __attribute__((aligned(bytes)))
	// move this to somewhere else, maybe BeBuild.h?


#ifndef __x86_64__
struct farcall {
	uint32* esp;
	uint32* ss;
};
#endif


// architecture specific thread info
struct arch_thread {
#ifdef __x86_64__
	// Back pointer to the containing Thread structure. The GS segment base is
	// pointed here, used to get the current thread.
	BKernel::Thread* thread;

	// RSP for kernel entry used by SYSCALL, and temporary scratch space.
	uint64*			syscall_rsp;
	uint64*			user_rsp;

	uint64*			current_stack;
#else
	struct farcall	current_stack;
	struct farcall	interrupt_stack;
#endif

	// 512 byte floating point save point - this must be 16 byte aligned
	uint8			fpu_state[512] _ALIGNED(16);

	addr_t			GetFramePointer() const;
} _ALIGNED(16);


struct arch_team {
	// gcc treats empty structures as zero-length in C, but as if they contain
	// a char in C++. So we have to put a dummy in to be able to use the struct
	// from both in a consistent way.
	char			dummy;
};


struct arch_fork_arg {
	struct iframe	iframe;
};


#ifdef __x86_64__


inline addr_t
arch_thread::GetFramePointer() const
{
	return current_stack[1];
}


#else


inline addr_t
arch_thread::GetFramePointer() const
{
	return current_stack.esp[2];
}


#endif	// __x86_64__

#endif	// _KERNEL_ARCH_x86_THREAD_TYPES_H
