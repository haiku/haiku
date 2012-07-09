/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>

#include "asm_offsets.h"


.text


/* void x86_fxsave(void* fpuState); */
FUNCTION(x86_fxsave):
	fxsave	(%rdi)
	ret
FUNCTION_END(x86_fxsave)


/* void x86_fxrstor(const void* fpuState); */
FUNCTION(x86_fxrstor):
	fxrstor	(%rdi)
	ret
FUNCTION_END(x86_fxrstor)


/* void x86_noop_swap(void *oldFpuState, const void *newFpuState); */
FUNCTION(x86_noop_swap):
	nop
	ret
FUNCTION_END(x86_noop_swap)


/* void x86_fxsave_swap(void* oldFpuState, const void* newFpuState); */
FUNCTION(x86_fxsave_swap):
	fxsave	(%rdi)
	fxrstor	(%rsi)
	ret
FUNCTION_END(x86_fxsave_swap)


/* addr_t x86_get_stack_frame(); */
FUNCTION(x86_get_stack_frame):
	mov		%rbp, %rax
	ret
FUNCTION_END(x86_get_stack_frame)


/* uint64 x86_read_msr(uint32 register); */
FUNCTION(x86_read_msr):
	mov		%edi, %ecx
	rdmsr
	shl		$32, %rdx
	mov		%eax, %eax
	or		%rdx, %rax
	ret
FUNCTION_END(x86_read_msr)


/* void x86_write_msr(uint32 register, uint64 value); */
FUNCTION(x86_write_msr):
	mov		%rsi, %rdx
	mov		%esi, %eax
	mov		%edi, %ecx
	shr		$32, %rdx
	wrmsr
	ret
FUNCTION_END(x86_write_msr)


/* void x86_64_thread_entry(); */
FUNCTION(x86_64_thread_entry):
	movq	%r15, %rdi
	jmp		*%r14
FUNCTION_END(x86_64_thread_entry)


/* void x86_context_switch(struct arch_thread* oldState,
	struct arch_thread* newState); */
FUNCTION(x86_context_switch):
	// Just need to save callee-save registers: RBP, RBX, R12-15.
	push	%r15
	push	%r14
	push	%r13
	push	%r12
	push	%rbp
	push	%rbx

	// Swap the stack pointers.
	movq	%rsp, ARCH_THREAD_current_stack(%rdi)
	movq	ARCH_THREAD_current_stack(%rsi), %rsp

	// Restore callee-save registers.
	pop		%rbx
	pop		%rbp
	pop		%r12
	pop		%r13
	pop		%r14
	pop		%r15

	ret
FUNCTION_END(x86_context_switch)


/* void x86_swap_pgdir(uint64 newPageDir); */
FUNCTION(x86_swap_pgdir):
	movq	%rdi, %cr3
	ret
FUNCTION_END(x86_swap_pgdir)


null_idt_descr:
	.word	0
	.quad	0

FUNCTION(x86_reboot):
	lidt	null_idt_descr
	int		$0
done:
	jmp		done
FUNCTION_END(x86_reboot)
