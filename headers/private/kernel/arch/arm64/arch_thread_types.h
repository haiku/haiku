/*
 * Copyright 2022, Haiku Inc. All rights reserved.
 * Copyright 2018, Jaroslaw Pelczar <jarek@jpelczar.com>
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_ARM64_ARCH_THREAD_TYPES_H_
#define _KERNEL_ARCH_ARM64_ARCH_THREAD_TYPES_H_


#include <kernel.h>

#define	IFRAME_TRACE_DEPTH 4

struct iframe_stack {
	struct iframe *frames[IFRAME_TRACE_DEPTH];
	int32	index;
};


struct arch_thread {
	uint64 regs[14]; // x19-x30, sp, tpidr_el0
	uint64 fp_regs[8]; // d8-d15

	// used to track interrupts on this thread
	struct iframe_stack	iframes;
};

struct arch_team {
	int			dummy;
};

struct arch_fork_arg {
	int			dummy;
};

#endif /* _KERNEL_ARCH_ARM64_ARCH_THREAD_TYPES_H_ */
