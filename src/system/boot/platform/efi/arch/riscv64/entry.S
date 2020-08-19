/*
 * Copyright 2011, François Revol <revol@free.fr>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <asm_defs.h>


    .text

/*  status_t arch_enter_kernel(struct kernel_args *kernelArgs,
        addr_t kernelEntry, addr_t kernelStackTop);

    r0  - kernelArgs
    r1  - kernelEntry
    r2  - kernelStackTop
*/
FUNCTION(arch_enter_kernel):
	nop

//	// set the kernel stack
//	mov     sp,r2
//
//	// set up kernel _start args
//	//mov   r0,r0   // kernelArgs
//	mov     r4,r1
//	mov     r1,#0   // currentCPU=0
//
//	// call the kernel
//	mov     pc,r4
//
//	// return
//	mov     r0,#-1  // B_ERROR
//	mov     pc,lr
FUNCTION_END(arch_enter_kernel)
