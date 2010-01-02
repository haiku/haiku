/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#define FUNCTION(x) .global x; .type x,@function; x

/*	status_t arch_start_kernel(struct kernel_args *kernelArgs,
		addr_t kernelEntry, addr_t kernelStackTop);

	r3	- kernelArgs
	r4	- kernelEntry
	r5	- kernelStackTop
*/
FUNCTION(arch_start_kernel):
	// push a stack frame
	stwu	%r1, -32(%r1)
	mflr	%r0
	stw		%r0, 36(%r1)

	// save the old stack pointer in r29
	stw		%r29, 20(%r1)
	mr		%r29, %r1

	// set up the kernel stack
	subi	%r1, %r5, 16

	// clear the pointer to the previous frame
	li		%r0, 0
	stw		%r0, 0(%r1)

	// enter the kernel
	mtlr	%r4
	li		%r4, 0
	blrl

	/* Actually we should never get here, but at least for debugging purposes
	   it's quite nice to return in an orderly manner. */

	// reset the boot loader stack
	mr		%r1, %r29
	lwz		%r29, 20(%r1)

	// pop the stack frame
	lwz		%r0, 36(%r1)
	mtlr	%r0
	addi	%r1, %r1, 32
	blr

