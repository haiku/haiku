/*
 * Copyright 2020-2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <asm_defs.h>

    .text

/*  status_t arch_enter_kernel(struct kernel_args* kernelArgs,
        addr_t kernelEntry, addr_t kernelStackTop);

    a0  - kernelArgs
    a1  - kernelEntry
    a2  - kernelStackTop
*/
FUNCTION(arch_enter_kernel):
	// set the kernel stack
	mv	sp,a2

	// Setup kernel args
	//mv	a0,a0	// kernelArgs
	mv	a4,a1
	li	a1,0	// currentCPU=0

	// call the kernel
	jr	a4

	// return
	li	a0,-1  // B_ERROR
	ret
FUNCTION_END(arch_enter_kernel)
