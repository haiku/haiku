/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef KERNEL_ARCH_ARM_THREAD_TYPES_H
#define KERNEL_ARCH_ARM_THREAD_TYPES_H

#include <kernel.h>

#define	IFRAME_TRACE_DEPTH 4

struct iframe_stack {
	struct iframe *frames[IFRAME_TRACE_DEPTH];
	int32	index;
};

struct arch_fpu_context {
	uint64_t	fp_regs[32];
	uint32_t	fpscr;
};

// architecture specific thread info
struct arch_thread {
	void	*sp;	// stack pointer
	struct	arch_fpu_context fpuContext;

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


#ifdef __cplusplus
extern "C" {
#endif

void arch_return_to_userland(struct iframe *);
void arm_context_switch(struct arch_thread* from, struct arch_thread* to);
void arm_save_fpu(struct arch_fpu_context* context);
void arm_restore_fpu(struct arch_fpu_context* context);

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_ARCH_ARM_THREAD_TYPES_H */
