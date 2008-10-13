/* 
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>. All rights
 * reserved. Distributed under the terms of the Haiku License.
 */

#include <asm_defs.h>

#include "setjmp_internal.h"

/* int sigsetjmp(jmp_buf buffer, int saveMask) */
FUNCTION(__sigsetjmp):
FUNCTION(sigsetjmp):
	// a0: buffer, d0: saveMask
	move.l	(4,%a7),%a0
	move.l	(8,%a7),%d0

	// store non-volatile general purpose registers
#warning M68K:	 do we want to save a7 there ??
	movem.l	%d2-%d7/%a2-%a7,(%a0)

	// store special registers (link, condition)
	clr.l	%d0
	move	%ccr,%d0
	move.w	%d0,(JMP_REGS_CCR,%a0)
	move.l	(%a7),(JMP_REGS_PC,%a0)

#warning M68K: check this.

	jmp		__setjmp_save_sigs
FUNCTION_END(__sigsetjmp)
FUNCTION_END(sigsetjmp)


/* int setjmp(jmp_buf buffer) */
FUNCTION(setjmp):
	move.l	(%a7)+,%a0
	move.l	(%a7)+,%d0
	clr.l	-(%a7)		/* push 0 as 2nd arg */
	move.l	%d0,-(%a7)
	move.l	%a0,-(%a7)
	// call __sigsetjmp with saveMask = 0
	jmp	__sigsetjmp
FUNCTION_END(setjmp)

#pragma weak _setjmp=setjmp
