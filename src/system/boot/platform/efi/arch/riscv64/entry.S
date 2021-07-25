/*
 * Copyright 2020-2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <asm_defs.h>

    .text

/*  status_t arch_enter_kernel(addr_t satp, struct kernel_args *kernelArgs,
        addr_t kernelEntry, addr_t kernelStackTop);

    a0  - SATP register value
    a1  - kernelArgs
    a2  - kernelEntry
    a3  - kernelStackTop
*/
FUNCTION(arch_enter_kernel):
	csrw satp, a0
	sfence.vma

	// set the kernel stack
	mv	sp, a3
	li	fp, 0
	li  ra, 0

	// Setup kernel args
	mv	a0, a1	// kernelArgs
	mv	t0, a2
	li	a1, 0	// currentCPU=0

	// call the kernel
	jr	t0

	// return
	li	a0,-1  // B_ERROR
	ret
FUNCTION_END(arch_enter_kernel)
