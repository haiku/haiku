/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef KERNEL_ARCH_PPC_THREAD_TYPES_H
#define KERNEL_ARCH_PPC_THREAD_TYPES_H


#include <arch_cpu.h>


#define	IFRAME_TRACE_DEPTH 4

struct iframe_stack {
	struct iframe *frames[IFRAME_TRACE_DEPTH];
	int32	index;
};

// architecture specific thread info
struct arch_thread {
	void	*sp;	// stack pointer
	void	*interrupt_stack;

	// used to track interrupts on this thread
	struct iframe_stack	iframes;
};

struct arch_team {
	// gcc treats empty structures as zero-length in C, but as if they contain
	// a char in C++. So we have to put a dummy in to be able to use the struct
	// from both in a consistent way.
	char	dummy;
};

struct arch_fork_arg {
	// gcc treats empty structures as zero-length in C, but as if they contain
	// a char in C++. So we have to put a dummy in to be able to use the struct
	// from both in a consistent way.
	char	dummy;
};

#endif	/* KERNEL_ARCH_PPC_THREAD_TYPES_H */
