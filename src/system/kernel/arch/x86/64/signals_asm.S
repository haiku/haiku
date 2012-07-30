/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>

#include "asm_offsets.h"
#include "syscall_numbers.h"


.text


// Userspace signal handler wrapper, copied to the commpage.
FUNCTION(x86_64_user_signal_handler):
	push	%rbp
	movq	%rsp, %rbp

	// RDI points to the signal_frame_data structure, however we'll overwrite
	// that with the function arguments, so move it somewhere else. We can use
	// callee-save registers here without preserving them because the old value
	// will be restored when the frame is restored.
	movq	%rdi, %r12

	// Check the handler type.
	cmpb	$0, SIGNAL_FRAME_DATA_siginfo_handler(%r12)
	jne		.Lsiginfo_handler

.Lsimple_handler:
	// Fetch other arguments (user data, vregs).
	movq	SIGNAL_FRAME_DATA_user_data(%r12), %rsi
	leaq	SIGNAL_FRAME_DATA_context + UCONTEXT_T_uc_mcontext(%r12), %rdx

.Lcall_handler:
	// Get the handler address and the signal number first argument.
	movq	SIGNAL_FRAME_DATA_handler(%r12), %rax
	movl	SIGNAL_FRAME_DATA_info + SIGINFO_T_si_signo(%r12), %edi

	// Call the handler.
	callq	*%rax

	// Perform the restore_signal_frame() syscall, should not return.
	movq	$SYSCALL_RESTORE_SIGNAL_FRAME, %rax
	movq	%r12, %rdi
	syscall

.Lsiginfo_handler:
	// Fetch other arguments (info pointer, context, user data).
	leaq	SIGNAL_FRAME_DATA_info(%r12), %rsi
	leaq	SIGNAL_FRAME_DATA_context(%r12), %rdx
	movq	SIGNAL_FRAME_DATA_user_data(%r12), %rcx
	jmp		.Lcall_handler
FUNCTION_END(x86_64_user_signal_handler)
SYMBOL(x86_64_user_signal_handler_end):
