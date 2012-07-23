/*
 * Copyright 2002-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ARCH_SIGNAL_H_
#define _ARCH_SIGNAL_H_


/*
 * Architecture-specific structure passed to signal handlers
 */

#if __x86_64__

struct vregs {
	unsigned long			rax;	/* gp regs */
	unsigned long			rdx;
	unsigned long			rcx;
	unsigned long			rbx;
	unsigned long			rsi;
	unsigned long			rdi;
	unsigned long			rbp;
	unsigned long			rsp;

	unsigned long			r8;	/* egp regs */
	unsigned long			r9;
	unsigned long			r10;
	unsigned long			r11;
	unsigned long			r12;
	unsigned long			r13;
	unsigned long			r14;
	unsigned long			r15;

	unsigned long			rip;

/*TODO:	add
*	Floatpoint
*	MMX
*	SSE
*/

};

#endif /* __x86_64__ */

#endif /* _ARCH_SIGNAL_H_ */
