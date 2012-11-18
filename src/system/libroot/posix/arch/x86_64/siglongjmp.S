/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */

#include "setjmp_internal.h"


/* int __siglongjmp(jmp_buf buffer, int value) */
FUNCTION(siglongjmp):
FUNCTION(longjmp):
FUNCTION(_longjmp):
	// Restore new callee-save registers.
	movq	JMP_REGS_R15(%rdi), %r15
	movq	JMP_REGS_R14(%rdi), %r14
	movq	JMP_REGS_R13(%rdi), %r13
	movq	JMP_REGS_R12(%rdi), %r12
	movq	JMP_REGS_BX(%rdi), %rbx
	movq	JMP_REGS_BP(%rdi), %rbp

	// Restore new stack pointer and push return address.
	movq	JMP_REGS_SP(%rdi), %rsp
	movq	JMP_REGS_IP(%rdi), %rax
	push	%rax

	// __longjmp_return restores the signal mask and sets the return value.
	call	__longjmp_return@PLT
	ret
FUNCTION_END(siglongjmp)


#pragma weak longjmp=siglongjmp
