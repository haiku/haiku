/*
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_ARCH_X86_32_IFRAME_H
#define _KERNEL_ARCH_X86_32_IFRAME_H


struct iframe {
	uint32 type;	// iframe type
	uint32 gs;
	uint32 fs;
	uint32 es;
	uint32 ds;
	uint32 di;
	uint32 si;
	uint32 bp;
	uint32 sp;
	uint32 bx;
	uint32 dx;
	uint32 cx;
	uint32 ax;
	uint32 orig_eax;
	uint32 orig_edx;
	uint32 vector;
	uint32 error_code;
	uint32 ip;
	uint32 cs;
	uint32 flags;

	// user_sp and user_ss are only present when the iframe is a userland
	// iframe (IFRAME_IS_USER()). A kernel iframe is shorter.
	uint32 user_sp;
	uint32 user_ss;
};

#define IFRAME_IS_USER(f)	((f)->cs == USER_CODE_SEG \
								|| ((f)->flags & 0x20000) != 0)


#endif	/* _KERNEL_ARCH_X86_32_IFRAME_H */
