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


#define _ALIGNED(bytes) __attribute__((aligned(bytes)))
	// move this to somewhere else, maybe BeBuild.h?


struct farcall {
	uint32* esp;
	uint32* ss;
};


// architecture specific thread info
struct arch_thread {
#ifdef __x86_64__
	uint64* rsp;
#else
	struct farcall current_stack;
	struct farcall interrupt_stack;
#endif

	// 512 byte floating point save point - this must be 16 byte aligned
	uint8 fpu_state[512] _ALIGNED(16);
} _ALIGNED(16);


struct arch_team {
	// gcc treats empty structures as zero-length in C, but as if they contain
	// a char in C++. So we have to put a dummy in to be able to use the struct
	// from both in a consistent way.
	char	dummy;
};


struct arch_fork_arg {
	struct iframe	iframe;
};


#endif	/* _KERNEL_ARCH_x86_THREAD_TYPES_H */
