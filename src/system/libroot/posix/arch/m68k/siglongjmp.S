/* 
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>. All rights
 * reserved. Distributed under the terms of the Haiku License.
 */

#include <asm_defs.h>

#include "setjmp_internal.h"

/* int __siglongjmp(jmp_buf buffer, int value) */
FUNCTION(siglongjmp):
FUNCTION(longjmp):
FUNCTION(_longjmp):
	// a0: buffer, d0: saveMask
	move.l	(4,%a7),%a0
	move.l	(8,%a7),%d0

	// restore non-volatile general purpose registers
	movem.l	(%a0),%d2-%d7/%a2-%a7

	// restore special registers (link, condition)
	move.w	(JMP_REGS_CCR,%a0),%d0
	move	%d0,%ccr
	move.l	(JMP_REGS_PC,%a0),(%a7)

	jmp	__longjmp_return
FUNCTION_END(siglongjmp)
FUNCTION_END(longjmp)
FUNCTION_END(_longjmp)

#pragma weak longjmp=siglongjmp
