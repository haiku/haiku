/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <asm_defs.h>


call_stub:
	// push a pointer to arguments and a pointer to ourselves on the stack
	lea		4(%esp), %eax
	push	%eax
	call	1f
1:
	pop		%eax
	subl	$(1b - call_stub), %eax
	push	%eax

	// call the wrapper function
	movl	(call_stub_callback_address - call_stub)(%eax), %eax
	call	*%eax
				// returns a pointer to the actual function
	lea		8(%esp), %esp

	jmp		*%eax

.align 4
call_stub_callback_address:
	.long	0
call_stub_end:


// size_t arch_call_stub_size();
FUNCTION(arch_call_stub_size):
	movl	$(call_stub_end - call_stub), %eax
	ret
FUNCTION_END(arch_call_stub_size)



// void arch_init_call_stub(void* stub,
//		void* (*callback)(const void* stub, const void* args),
//		void* function);
FUNCTION(arch_init_call_stub):
	push	%ebp
	movl	%esp, %ebp

	// stub address to %edi
	push	%edi
	movl	8(%ebp), %edi

	// copy the stub
	movl	$(call_stub_end - call_stub), %eax
	push	%eax
	movl	$call_stub, %eax
	push	%eax
	push	%edi
	call	memcpy
	lea		12(%esp), %esp

	// set the callback address in the stub
	movl	12(%ebp), %eax
	movl	%eax, (call_stub_callback_address - call_stub)(%edi)

	// restore %edi
	pop		%edi

	movl	%ebp, %esp
	pop		%ebp
	ret
FUNCTION_END(arch_init_call_stub)
