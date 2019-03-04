/*
 * Copyright 2019, Jérôme Duval, jerome.duval@gmail.com.
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <asm_defs.h>


call_stub:
	// push arguments on the stack
	push	%r9
	push	%r8
	push	%rcx
	push	%rdx
	push	%rsi
	push	%rdi
	// pointer to ourself
	lea		call_stub(%rip), %rdi
	// pointer to first argument
	mov		%rsp, %rsi

	// call the wrapper function
	movq	(call_stub_callback_address - call_stub)(%rdi), %rax
	call	*%rax
				// returns a pointer to the actual function
	// restore arguments before calling
	pop		%rdi
	pop		%rsi
	pop		%rdx
	pop		%rcx
	pop		%r8
	pop		%r9
	jmp		*%rax

.align 8
call_stub_callback_address:
	.long	0
call_stub_end:


// size_t arch_call_stub_size();
FUNCTION(arch_call_stub_size):
	movq	$(call_stub_end - call_stub), %rax
	ret
FUNCTION_END(arch_call_stub_size)


// void arch_init_call_stub(void* stub,
//		void* (*callback)(const void* stub, const void* args),
//		void* function);
FUNCTION(arch_init_call_stub):
	push	%rbp
	movq	%rsp, %rbp

	push	%rsi
	push	%rdi

	// copy the stub
	movq	$(call_stub_end - call_stub), %rdx
	lea		call_stub(%rip), %rsi
	call	memcpy@plt

	// set the callback address in the stub
	pop		%rdi
	pop		%rsi
	movq	%rsi, (call_stub_callback_address - call_stub)(%rdi)

	movq	%rbp, %rsp
	pop		%rbp
	ret
FUNCTION_END(arch_init_call_stub)
