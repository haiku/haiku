/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_X86_64_IFRAME_H
#define _KERNEL_ARCH_X86_64_IFRAME_H


struct iframe {
	uint64 type;
	uint64 r15;
	uint64 r14;
	uint64 r13;
	uint64 r12;
	uint64 r11;
	uint64 r10;
	uint64 r9;
	uint64 r8;
	uint64 rbp;
	uint64 rsi;
	uint64 rdi;
	uint64 rdx;
	uint64 rcx;
	uint64 rbx;
	uint64 rax;
	uint64 vector;
	uint64 error_code;
	uint64 rip;
	uint64 cs;
	uint64 flags;

	// Only present when the iframe is a userland iframe (IFRAME_IS_USER()).
	uint64 user_rsp;
	uint64 user_ss;
} _PACKED;

#define IFRAME_IS_USER(f)	(((f)->cs & DPL_USER) == DPL_USER)


#endif	/* _KERNEL_ARCH_X86_64_IFRAME_H */
