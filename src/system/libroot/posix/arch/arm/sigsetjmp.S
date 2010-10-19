/* 
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>. All rights
 * reserved. Distributed under the terms of the Haiku License.
 */

#include <asm_defs.h>

//#include "setjmp_internal.h"

/* int sigsetjmp(jmp_buf buffer, int saveMask) */
FUNCTION(__sigsetjmp):
FUNCTION(sigsetjmp):
	stmdb   sp!, {r4, lr}
        stmia   r0, {r0-r14}
        str     lr,[r0, #60]
        mrs     r1,cpsr
        str     r1,[r0,#64]
        ldr     r1,[r0,#4]
        mov     r0, #0

	bl		__setjmp_save_sigs
	ldmia   sp!, {r4, pc}
FUNCTION_END(__sigsetjmp)
FUNCTION_END(sigsetjmp)


/* int setjmp(jmp_buf buffer) */
FUNCTION(setjmp):
        stmia   r0, {r0-r14}
        str     lr,[r0, #60]
        mrs     r1,cpsr
        str     r1,[r0,#64]
        ldr     r1,[r0,#4]
        mov     r0, #0
	bx	lr
FUNCTION_END(setjmp)

#pragma weak _setjmp=setjmp
