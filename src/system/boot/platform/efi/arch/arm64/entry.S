/*
 * Copyright 2020-2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <asm_defs.h>


	.text

/*	status_t arch_enter_kernel(struct kernel_args *kernelArgs,
		addr_t kernelEntry, addr_t kernelStackTop, uint32 cpu);

	x0	- kernelArgs
	x1	- kernelEntry
	x2	- kernelStackTop
	x3	- cpu
*/
FUNCTION(arch_enter_kernel):

	// set the kernel stack
	mov		sp,x2

	// set up kernel _start args
	//mov	x0,x0	// kernelArgs
	mov		x4,x1
	mov		x1,x3	// currentCPU

	// clear frame pointer to ensure the kernel doesn't
	// try to continue a stacktrace back out here
	mov		fp,#0

	// call the kernel
	br		x4

	// return
	mov		x0,#-1	// B_ERROR
	ret

FUNCTION_END(arch_enter_kernel)

