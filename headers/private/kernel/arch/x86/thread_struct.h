/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _NEWOS_KERNEL_ARCH_I386_THREAD_STRUCT_H
#define _NEWOS_KERNEL_ARCH_I386_THREAD_STRUCT_H

// architecture specific thread info
struct arch_thread {
	unsigned int *esp;
	// 512 byte floating point save point
	uint8 fpu_state[512];
};

struct arch_proc {
	// nothing here
};

#endif

