/*
 * Copyright 2020-2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <asm_defs.h>


	.text

/*	status_t arch_enter_kernel(struct kernel_args *kernelArgs,
		addr_t kernelEntry, addr_t kernelStackTop);

	x0	- kernelArgs
	x1	- kernelEntry
	x2	- kernelStackTop
*/
FUNCTION(arch_enter_kernel):

	// set the kernel stack
	mov		sp,x2

	// set up kernel _start args
	//mov	x0,x0	// kernelArgs
	mov		x4,x1
	mov		x1,#0	// currentCPU=0

	// call the kernel
	b		x4

	// return
	mov		x0,#-1	// B_ERROR
	ret

FUNCTION_END(arch_enter_kernel)

