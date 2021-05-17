/* 
 * Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>


# TODO: FPU?

/* int __siglongjmp(jmp_buf buffer, int value) */
FUNCTION(siglongjmp):
FUNCTION(longjmp):
FUNCTION(_longjmp):
	ld ra,   0*8(a0)
	ld s0,   1*8(a0)
	ld s1,   2*8(a0)
	ld s2,   3*8(a0)
	ld s3,   4*8(a0)
	ld s4,   5*8(a0)
	ld s5,   6*8(a0)
	ld s6,   7*8(a0)
	ld s7,   8*8(a0)
	ld s8,   9*8(a0)
	ld s9,  10*8(a0)
	ld s10, 11*8(a0)
	ld s11, 12*8(a0)
	ld sp,  13*8(a0)

	tail __longjmp_return
FUNCTION_END(siglongjmp)
FUNCTION_END(longjmp)
FUNCTION_END(_longjmp)

#pragma weak longjmp=siglongjmp
