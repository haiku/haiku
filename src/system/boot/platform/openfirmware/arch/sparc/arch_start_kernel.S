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
	return
