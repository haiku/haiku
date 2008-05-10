/* 
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>. All rights
 * reserved. Distributed under the terms of the Haiku License.
 */

#include "setjmp_internal.h"

/* int sigsetjmp(jmp_buf buffer, int saveMask) */
FUNCTION(__sigsetjmp):
FUNCTION(sigsetjmp):
	// r3: buffer, r4: saveMask

	// store non-volatile general purpose registers
	stw		%r1, JMP_REGS_R1(3)
	stw		%r2, JMP_REGS_R2(3)
	stw		%r13, JMP_REGS_R13(3)
	stw		%r14, JMP_REGS_R14(3)
	stw		%r15, JMP_REGS_R15(3)
	stw		%r16, JMP_REGS_R16(3)
	stw		%r17, JMP_REGS_R17(3)
	stw		%r18, JMP_REGS_R18(3)
	stw		%r19, JMP_REGS_R19(3)
	stw		%r20, JMP_REGS_R20(3)
	stw		%r21, JMP_REGS_R21(3)
	stw		%r22, JMP_REGS_R22(3)
	stw		%r23, JMP_REGS_R23(3)
	stw		%r24, JMP_REGS_R24(3)
	stw		%r25, JMP_REGS_R25(3)
	stw		%r26, JMP_REGS_R26(3)
	stw		%r27, JMP_REGS_R27(3)
	stw		%r28, JMP_REGS_R28(3)
	stw		%r29, JMP_REGS_R29(3)
	stw		%r30, JMP_REGS_R30(3)
	stw		%r31, JMP_REGS_R31(3)

	// store special registers (link, condition)
	mflr	%r0
	stw		%r0, JMP_REGS_LR(3)
	mfcr	%r0
	stw		%r0, JMP_REGS_CR(3)

	b		__setjmp_save_sigs


/* int setjmp(jmp_buf buffer) */
FUNCTION(setjmp):
	// call __sigsetjmp with saveMask = 0
	addi	%r4, 0, 0
	b		__sigsetjmp

#pragma weak _setjmp=setjmp
