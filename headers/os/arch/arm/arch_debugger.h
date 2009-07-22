/*
 * Copyright 2005, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ARCH_ARM_DEBUGGER_H
#define _ARCH_ARM_DEBUGGER_H

struct arm_debug_cpu_state {
        ulong r0;
        ulong r1;
        ulong r2;
        ulong r3;
        ulong r4;
        ulong r5;
        ulong r6;
        ulong r7;
        ulong r8;
        ulong r9;
        ulong r10;
        ulong r11;
        ulong r12;
        ulong r13;      /* stack pointer */
        ulong r14;      /* link register */
        ulong r15;      /* program counter */
        ulong cpsr;
#if __ARM__
#warning ARM: missing members!
#endif
	uint32	dummy;
} __attribute__((aligned(8)));

#endif	// _ARCH_ARM_DEBUGGER_H
