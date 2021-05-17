/* Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk.
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>


# TODO: FPU?

/* int sigsetjmp(jmp_buf buffer, int saveMask) */
FUNCTION(__sigsetjmp):
FUNCTION(sigsetjmp):
	sd ra,   0*8(a0)
	sd s0,   1*8(a0)
	sd s1,   2*8(a0)
	sd s2,   3*8(a0)
	sd s3,   4*8(a0)
	sd s4,   5*8(a0)
	sd s5,   6*8(a0)
	sd s6,   7*8(a0)
	sd s7,   8*8(a0)
	sd s8,   9*8(a0)
	sd s9,  10*8(a0)
	sd s10, 11*8(a0)
	sd s11, 12*8(a0)
	sd sp,  13*8(a0)

	tail __setjmp_save_sigs
FUNCTION_END(setjmp)


/* int setjmp(jmp_buf buffer) */
FUNCTION(setjmp):
FUNCTION(_setjmp):
	li a1, 0
	tail sigsetjmp
FUNCTION_END(setjmp)
