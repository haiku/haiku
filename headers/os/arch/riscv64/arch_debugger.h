/*
 * Copyright 2009-2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ARCH_RISCV64_DEBUGGER_H
#define _ARCH_RISCV64_DEBUGGER_H


struct riscv64_debug_cpu_state {
	uint64 x[31];
	uint64 pc;
	double f[32];
	uint64 fcsr;
} __attribute__((aligned(8)));


#endif	// _ARCH_RISCV64_DEBUGGER_H
