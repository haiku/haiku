/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_ARCH_x86_THREAD_STRUCT_H
#define _KERNEL_ARCH_x86_THREAD_STRUCT_H

struct farcall {
	unsigned int *esp;
	unsigned int *ss;
};

// architecture specific thread info
struct arch_thread {
	struct farcall current_stack;
	struct farcall interrupt_stack;
	// 512 byte floating point save point
	uint8 fpu_state[512];
};

struct arch_team {
	// nothing here
};

#endif

