/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef KERNEL_ARCH_PPC_THREAD_STRUCT_H
#define KERNEL_ARCH_PPC_THREAD_STRUCT_H

// architecture specific thread info
struct arch_thread {
	void *sp;	// stack pointer
};

struct arch_team {
	// nothing here
};

#endif	/* KERNEL_ARCH_PPC_THREAD_STRUCT_H */
