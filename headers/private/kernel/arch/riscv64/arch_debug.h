/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_RISCV64_DEBUG_H
#define _KERNEL_ARCH_RISCV64_DEBUG_H


#include <SupportDefs.h>


struct kernel_args;
struct iframe;

struct arch_debug_registers {
	addr_t	fp;
};


status_t arch_debug_init_early(kernel_args *args);


#endif	// _KERNEL_ARCH_RISCV64_DEBUG_H
