/*
 * Copyright 2009-2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ARCH_RISCV64_DEBUGGER_H
#define _ARCH_RISCV64_DEBUGGER_H


//#warning RISCV64: fixme
struct riscv64_debug_cpu_state {
	uint32	dummy;
} __attribute__((aligned(8)));


#endif	// _ARCH_RISCV64_DEBUGGER_H
