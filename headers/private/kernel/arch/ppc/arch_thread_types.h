/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef KERNEL_ARCH_PPC_THREAD_TYPES_H
#define KERNEL_ARCH_PPC_THREAD_TYPES_H

// architecture specific thread info
struct arch_thread {
	void *sp;	// stack pointer
};

struct arch_team {
	// nothing here
};

struct arch_fork_arg {
	// nothing here yet
};

#endif	/* KERNEL_ARCH_PPC_THREAD_TYPES_H */
