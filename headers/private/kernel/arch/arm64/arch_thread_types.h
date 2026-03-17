/*
 * Copyright 2022, Haiku Inc. All rights reserved.
 * Copyright 2018, Jaroslaw Pelczar <jarek@jpelczar.com>
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_ARM64_ARCH_THREAD_TYPES_H_
#define _KERNEL_ARCH_ARM64_ARCH_THREAD_TYPES_H_


#include <kernel.h>

#define	IFRAME_TRACE_DEPTH 4


struct aarch64_fpu_state
{
	uint64 regs[32 * 2];
	uint64 fpsr;
	uint64 fpcr;
};


/* raw exception frames */
struct iframe {
	// return info
	uint64 elr;
	uint64 spsr;
	uint64 x[29];
	uint64 fp;
	uint64 lr;
	uint64 sp;
	uint64 tpidr;

	// exception info
	uint64 esr;
	uint64 far;

	// fpu
	struct aarch64_fpu_state fpu;
};


struct iframe_stack {
	struct iframe *frames[IFRAME_TRACE_DEPTH];
	int32	index;
};


struct arch_thread {
	uint64 regs[14]; // x19-x30, sp, tpidr_el0
	uint64 fp_regs[8]; // d8-d15
	uint64 old_x0;

	// used to track interrupts on this thread
	struct iframe_stack	iframes;
};


struct arch_team {
	int			dummy;
};


struct arch_fork_arg {
	struct iframe frame;
};


#endif /* _KERNEL_ARCH_ARM64_ARCH_THREAD_TYPES_H_ */
