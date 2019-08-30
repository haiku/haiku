/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ARCH_ARM64_DEBUGGER_H
#define _ARCH_ARM64_DEBUGGER_H

struct arm64_debug_cpu_state {
	unsigned long x[30];
	unsigned long lr;
	unsigned long sp;
	unsigned long elr;
	unsigned int  spsr;
} __attribute__((aligned(16)));

#endif	// _ARCH_ARM_DEBUGGER_H
