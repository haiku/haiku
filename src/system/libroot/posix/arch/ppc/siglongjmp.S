/* 
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>. All rights
 * reserved. Distributed under the terms of the Haiku License.
 */

#include "setjmp_internal.h"

/* int __siglongjmp(jmp_buf buffer, int value) */
FUNCTION(siglongjmp):
FUNCTION(longjmp):
FUNCTION(_longjmp):
	// r3: buffer, r4: saveMask

	// restore non-volatile general purpose registers
	lwz		%r1, JMP_REGS_R1(3)
	lwz		%r2, JMP_REGS_R2(3)
	lwz		%r13, JMP_REGS_R13(3)
	lwz		%r14, JMP_REGS_R14(3)
	lwz		%r15, JMP_REGS_R15(3)
	lwz		%r16, JMP_REGS_R16(3)
	lwz		%r17, JMP_REGS_R17(3)
	lwz		%r18, JMP_REGS_R18(3)
	lwz		%r19, JMP_REGS_R19(3)
	lwz		%r20, JMP_REGS_R20(3)
	lwz		%r21, JMP_REGS_R21(3)
	lwz		%r22, JMP_REGS_R22(3)
	lwz		%r23, JMP_REGS_R23(3)
	lwz		%r24, JMP_REGS_R24(3)
	lwz		%r25, JMP_REGS_R25(3)
	lwz		%r26, JMP_REGS_R26(3)
	lwz		%r27, JMP_REGS_R27(3)
	lwz		%r28, JMP_REGS_R28(3)
	lwz		%r29, JMP_REGS_R29(3)
	lwz		%r30, JMP_REGS_R30(3)
	lwz		%r31, JMP_REGS_R31(3)

	// restore special registers (link, condition)
	lwz		%r0, JMP_REGS_LR(3)
	mtlr	%r0
	lwz		%r0, JMP_REGS_CR(3)
	mtcr	%r0

	b		__longjmp_return

#pragma weak longjmp=siglongjmp
