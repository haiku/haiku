/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_X86_64_CPU_H
#define _KERNEL_ARCH_X86_64_CPU_H


#include "../common_x86/cpu.h"

#ifndef _ASSEMBLER
# ifndef _BOOT_MODE
#  include <arch/x86_64/descriptors.h>
# endif
#endif


// iframe types
#define IFRAME_TYPE_SYSCALL				0x1
#define IFRAME_TYPE_OTHER				0x2
#define IFRAME_TYPE_MASK				0xf


#ifndef _ASSEMBLER


struct tss {
	uint32 _reserved1;
	uint64 rsp0;
	uint64 rsp1;
	uint64 rsp2;
	uint64 _reserved2;
	uint64 ist1;
	uint64 ist2;
	uint64 ist3;
	uint64 ist4;
	uint64 ist5;
	uint64 ist6;
	uint64 ist7;
	uint64 _reserved3;
	uint16 _reserved4;
	uint16 io_bitmap;
} _PACKED;


struct iframe {
	unsigned long type;
	unsigned long r15;
	unsigned long r14;
	unsigned long r13;
	unsigned long r12;
	unsigned long r11;
	unsigned long r10;
	unsigned long r9;
	unsigned long r8;
	unsigned long rbp;
	unsigned long rsi;
	unsigned long rdi;
	unsigned long rdx;
	unsigned long rcx;
	unsigned long rbx;
	unsigned long rax;
	unsigned long vector;
	unsigned long error_code;
	unsigned long rip;
	unsigned long cs;
	unsigned long flags;

	// Only present when the iframe is a userland iframe (IFRAME_IS_USER()).
	unsigned long user_rsp;
	unsigned long user_ss;
} _PACKED;

#define IFRAME_IS_USER(f)	(((f)->cs & DPL_USER) == DPL_USER)


typedef struct arch_cpu_info {
	// CPU identification/feature information.
	enum x86_vendors	vendor;
	uint32				feature[FEATURE_NUM];
	char				model_name[49];
	const char*			vendor_name;
	int					type;
	int					family;
	int					extended_family;
	int					stepping;
	int					model;
	int					extended_model;

	// TSS for this CPU.
	struct tss			tss;
} arch_cpu_info;


#ifndef _BOOT_MODE
extern segment_descriptor* gGDT;
#endif


#endif	/* _ASSEMBLER */

#endif	/* _KERNEL_ARCH_X86_64_CPU_H */
