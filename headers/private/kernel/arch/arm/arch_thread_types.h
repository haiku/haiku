/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef KERNEL_ARCH_ARM_THREAD_TYPES_H
#define KERNEL_ARCH_ARM_THREAD_TYPES_H

#include <kernel.h>


/* raw exception frames */
struct iframe {
	uint32 spsr;
	uint32 r0;
	uint32 r1;
	uint32 r2;
	uint32 r3;
	uint32 r4;
	uint32 r5;
	uint32 r6;
	uint32 r7;
	uint32 r8;
	uint32 r9;
	uint32 r10;
	uint32 r11;
	uint32 r12;
	uint32 usr_sp;
	uint32 usr_lr;
	uint32 svc_sp;
	uint32 svc_lr;
	uint32 pc;
} _PACKED;

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

	struct iframe*	userFrame;
	uint32	oldR0;
};

struct arch_team {
	// gcc treats empty structures as zero-length in C, but as if they contain
	// a char in C++. So we have to put a dummy in to be able to use the struct
	// from both in a consistent way.
	char	dummy;
};

struct arch_fork_arg {
	struct iframe	frame;
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
