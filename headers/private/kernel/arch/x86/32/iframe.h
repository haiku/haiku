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
	uint32 edi;
	uint32 esi;
	uint32 ebp;
	uint32 esp;
	uint32 ebx;
	uint32 edx;
	uint32 ecx;
	uint32 eax;
	uint32 orig_eax;
	uint32 orig_edx;
	uint32 vector;
	uint32 error_code;
	uint32 eip;
	uint32 cs;
	uint32 flags;

	// user_esp and user_ss are only present when the iframe is a userland
	// iframe (IFRAME_IS_USER()). A kernel iframe is shorter.
	uint32 user_esp;
	uint32 user_ss;
};

struct vm86_iframe {
	uint32 type;	// iframe type
	uint32 __null_gs;
	uint32 __null_fs;
	uint32 __null_es;
	uint32 __null_ds;
	uint32 edi;
	uint32 esi;
	uint32 ebp;
	uint32 __kern_esp;
	uint32 ebx;
	uint32 edx;
	uint32 ecx;
	uint32 eax;
	uint32 orig_eax;
	uint32 orig_edx;
	uint32 vector;
	uint32 error_code;
	uint32 eip;
	uint16 cs, __csh;
	uint32 flags;
	uint32 esp;
	uint16 ss, __ssh;

	/* vm86 mode specific part */
	uint16 es, __esh;
	uint16 ds, __dsh;
	uint16 fs, __fsh;
	uint16 gs, __gsh;
};

#define IFRAME_IS_USER(f)	((f)->cs == USER_CODE_SEG \
								|| ((f)->flags & 0x20000) != 0)
#define IFRAME_IS_VM86(f)	(((f)->flags & 0x20000) != 0)


#endif	/* _KERNEL_ARCH_X86_32_IFRAME_H */
