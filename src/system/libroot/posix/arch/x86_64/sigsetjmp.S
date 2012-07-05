/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */

#include "setjmp_internal.h"


/* int sigsetjmp(jmp_buf buffer, int saveMask) */
FUNCTION(__sigsetjmp):
FUNCTION(sigsetjmp):
	// Save instruction/stack pointers.
	movq	(%rsp), %rax
	movq	%rax, JMP_REGS_IP(%rdi)
	leaq	8(%rsp), %rax
	movq	%rax, JMP_REGS_SP(%rdi)

	// Save callee-save registers.
	movq	%rbp, JMP_REGS_BP(%rdi)
	movq	%rbx, JMP_REGS_BX(%rdi)
	movq	%r12, JMP_REGS_R12(%rdi)
	movq	%r13, JMP_REGS_R13(%rdi)
	movq	%r14, JMP_REGS_R14(%rdi)
	movq	%r15, JMP_REGS_R15(%rdi)

	// __setjmp_save_sigs handles signal mask and return value.
	jmp		__setjmp_save_sigs
FUNCTION_END(setjmp)


/* int setjmp(jmp_buf buffer) */
FUNCTION(setjmp):
	// Jump to sigsetjmp with a zero saveMask.
	xorl	%esi, %esi
	jmp		sigsetjmp
FUNCTION_END(setjmp)


#pragma weak _setjmp=setjmp
