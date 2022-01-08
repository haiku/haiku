/*
 * Copyright 2011, François Revol <revol@free.fr>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <asm_defs.h>


	.text

/*	status_t arch_enter_kernel(uint32_t ttbr, struct kernel_args *kernelArgs,
		addr_t kernelEntry, addr_t kernelStackTop);

	r0	- ttbr
	r1	- kernelArgs
	r2	- kernelEntry
	r3	- kernelStackTop
*/
FUNCTION(arch_enter_kernel):


	mov		r5,r0
	mov		r4,r2

	// set up kernel _start args
	mov		r0,r1	// kernelArgs
	mov		r1,#0	// currentCPU=0

	// enable full access for coprocessors P10, P11
	// by setting the required flags in Access Control Register
	MRC p15, #0, r9, c1, c0, #2
	orr r9, r9, #0x00f00000
	MCR p15, #0, r9, c1, c0, #2

	// flush prefetch buffer
	mov r9, #0
	MCR p15, #0, r9, c7, c5, #4

	// enable FPU
	mov r9, #0x40000000
	FMXR FPEXC, r9

	// flush TLB
	MCR p15, 0, r1, c8, c7, 0

	// set TTBR0
	MCR p15, 0, r5, c2, c0, 0

	// initialize TTBCR to zero (no LPAE, use only TTBR0)
	MCR p15, 0, r1, c2, c0, 2

	// flush TLB (again)
	MCR p15, 0, r1, c8, c7, 0

	// write DACR
	mov r9, #0xffffffff
	MCR p15, 0, r9, c3, c0, 0

	// enable MMU
	MRC p15, 0, r9, c1, c0, 0
	orr r9, r9, #1
	MCR p15, 0, r9, c1, c0, 0

	// set the kernel stack
	mov		sp,r3

	// call the kernel
	mov		pc,r4

	// return
	mov		r0,#-1	// B_ERROR
	mov		pc,lr

FUNCTION_END(arch_enter_kernel)

