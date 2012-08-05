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
	uint64 bp;
	uint64 si;
	uint64 di;
	uint64 dx;
	uint64 cx;
	uint64 bx;
	uint64 ax;
	uint64 orig_rax;
	uint64 vector;
	uint64 error_code;
	uint64 ip;
	uint64 cs;
	uint64 flags;

	// SP and SS are unconditionally present on x86_64, make both names
	// available.
	union {
		uint64 sp;
		uint64 user_sp;
	};
	union {
		uint64 ss;
		uint64 user_ss;
	};
} _PACKED;

#define IFRAME_IS_USER(f)	(((f)->cs & DPL_USER) == DPL_USER)


#endif	/* _KERNEL_ARCH_X86_64_IFRAME_H */
