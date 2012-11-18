/*
 * Copyright 2002-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_ARCH_x86_DESCRIPTORS_H
#define _KERNEL_ARCH_x86_DESCRIPTORS_H


#ifndef _ASSEMBLER


#include <SupportDefs.h>


struct kernel_args;


enum descriptor_privilege_levels {
	DPL_KERNEL = 0,
	DPL_USER = 3,
};

enum descriptor_types {
	// segment types
	DT_CODE_EXECUTE_ONLY = 0x8,
	DT_CODE_ACCESSED = 0x9,
	DT_CODE_READABLE = 0xa,
	DT_CODE_CONFORM = 0xc,
	DT_DATA_READ_ONLY = 0x0,
	DT_DATA_ACCESSED = 0x1,
	DT_DATA_WRITEABLE = 0x2,
	DT_DATA_EXPANSION_DOWN = 0x4,

	DT_TSS = 9,
		/* non busy, 32 bit */

	// descriptor types
	DT_SYSTEM_SEGMENT = 0,
	DT_CODE_DATA_SEGMENT = 1,
};

enum gate_types {
	GATE_INTERRUPT = 14,
	GATE_TRAP = 15,
};


void x86_descriptors_init(kernel_args* args);
void x86_descriptors_init_percpu(kernel_args* args, int cpu);
status_t x86_descriptors_init_post_vm(kernel_args* args);


#endif	// !_ASSEMBLER


#ifdef __x86_64__
#	include <arch/x86/64/descriptors.h>
#else
#	include <arch/x86/32/descriptors.h>
#endif


#endif	/* _KERNEL_ARCH_x86_DESCRIPTORS_H */
