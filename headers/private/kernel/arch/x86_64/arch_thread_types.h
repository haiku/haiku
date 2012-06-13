/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_X86_64_THREAD_TYPES_H
#define _KERNEL_ARCH_X86_64_THREAD_TYPES_H


#include <arch_cpu.h>


// x86_64-specific thread information.
struct arch_thread {
	// Stack pointer. 
	addr_t rsp;

	// FPU saved state - this must be 16 byte aligned.
	uint8 fpu_state[512] __attribute__((aligned(16)));
} __attribute__((aligned(16)));

struct arch_team {
	// gcc treats empty structures as zero-length in C, but as if they contain
	// a char in C++. So we have to put a dummy in to be able to use the struct
	// from both in a consistent way.
	char dummy;
};

struct arch_fork_arg {
	struct iframe iframe;
};

#endif	/* _KERNEL_ARCH_X86_64_THREAD_TYPES_H */
