/*
 * Copyright 2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <asm_defs.h>

/* int sigsetjmp(jmp_buf buffer, int saveMask) */
FUNCTION(__sigsetjmp):
FUNCTION(sigsetjmp):
	stp x19, x20, [x0], #16
	stp x21, x22, [x0], #16
	stp x23, x24, [x0], #16
	stp x25, x26, [x0], #16
	stp x27, x28, [x0], #16
	stp x29, x30, [x0], #16

	mov x2, sp
	str x2, [x0], #8

	stp  d8,  d9, [x0], #16
	stp d10, d11, [x0], #16
	stp d12, d13, [x0], #16
	stp d14, d15, [x0], #16

	b __setjmp_save_sigs
FUNCTION_END(sigsetjmp)
FUNCTION_END(__sigsetjmp)


/* int setjmp(jmp_buf buffer) */
FUNCTION(setjmp):
FUNCTION(_setjmp):
	mov x1, #0
	b __sigsetjmp
FUNCTION_END(_setjmp)
FUNCTION_END(setjmp)

#pragma weak _setjmp=setjmp
