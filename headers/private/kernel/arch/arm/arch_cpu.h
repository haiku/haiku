/*
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/
#ifndef _KERNEL_ARCH_ARM_CPU_H
#define _KERNEL_ARCH_ARM_CPU_H

#ifndef _ASSEMBLER

#include <arch/arm/arch_thread_types.h>
#include <kernel.h>

/* raw exception frames */
struct iframe {
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
	uint32 r13;
	uint32 lr;
	uint32 pc;
	uint32 cpsr;
} _PACKED;

typedef struct arch_cpu_info {
} arch_cpu_info;

extern int arch_cpu_type;
extern int arch_fpu_type;
extern int arch_mmu_type;
extern int arch_platform;
extern int arch_machine;

#endif	// !_ASSEMBLER

#endif	/* _KERNEL_ARCH_ARM_CPU_H */
