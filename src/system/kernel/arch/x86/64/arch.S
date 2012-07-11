/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Copyright 2003-2007, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Copyright 2002, Michael Noisternig. All rights reserved.
 * Distributed under the terms of the NewOS License.
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


/* status_t arch_cpu_user_memcpy(void* to, const void* from, size_t size,
		addr_t* faultHandler) */
FUNCTION(arch_cpu_user_memcpy):
	// faultHandler -> r8, size -> rcx.
	movq	%rcx, %r8
	movq	%rdx, %rcx

	// Set the fault handler, preserve old in rax.
	movq	(%r8), %rax
	movq	$.L_user_memcpy_error, (%r8)

	// Move by quadwords.
	cld
	movq	%rcx, %r9
	shrq	$3, %rcx
	rep
	movsq

	// Move any remaining data by bytes.
	movq	%r9, %rcx
	andq	$7, %rcx
	rep
	movsb

	// Restore the old fault handler and return.
	movq	%rax, (%r8)
	xorl	%eax, %eax
	ret

.L_user_memcpy_error:
	// Restore the old fault handler. Return a generic error, the wrapper
	// routine will deal with it.
	movq	%rax, (%r8)
	movl	$-1, %eax
	ret
FUNCTION_END(arch_cpu_user_memcpy)


/* status_t arch_cpu_user_memset(void* to, char c, size_t count,
		addr_t* faultHandler) */
FUNCTION(arch_cpu_user_memset):
	// c -> al, faultHandler -> r8, size -> rcx.
	movw	%si, %ax
	movq	%rcx, %r8
	movq	%rdx, %rcx

	// Set the fault handler, preserve old in rdx.
	movq	(%r8), %rdx
	movq	$.L_user_memset_error, (%r8)

	rep
	stosb

	// Restore the old fault handler and return.
	movq	%rdx, (%r8)
	xorl	%eax, %eax
	ret

.L_user_memset_error:
	// Restore the old fault handler. Return a generic error, the wrapper
	// routine will deal with it.
	movq	%rdx, (%r8)
	movl	$-1, %eax
	ret
FUNCTION_END(arch_cpu_user_memset)


/* ssize_t arch_cpu_user_strlcpy(void* to, const void* from, size_t size,
		addr_t* faultHandler) */
FUNCTION(arch_cpu_user_strlcpy):
	// faultHandler -> r8, size -> rcx, source -> r9 (original value needed to
	// calculate return value).
	movq	%rcx, %r8
	movq	%rdx, %rcx
	movq	%rsi, %r9

	// Set the fault handler, preserve old in rax.
	movq	(%r8), %rax
	movq	$.L_user_strlcpy_error, (%r8)

	// Check for 0 length.
	cmp		$0, %rcx
	je		.L_user_strlcpy_source_count

	// Copy at most count - 1 bytes.
	dec		%rcx

	// If count is now 0, skip straight to null terminating as our loop will
	// otherwise overflow.
	jnz		.L_user_strlcpy_copy_begin
	movb	$0, (%rdi)
	jmp		.L_user_strlcpy_source_count

.L_user_strlcpy_copy_begin:
	cld
.L_user_strlcpy_copy_loop:
	// Move data by bytes.
	lodsb
	stosb
	test	%al, %al
	jz		.L_user_strlcpy_source_done
	loop	.L_user_strlcpy_copy_loop

	// Null terminate string.
	movb	$0, (%rdi)
	dec		%rsi

.L_user_strlcpy_source_count:
	// Count remaining bytes in src
	not		%rcx
	xor		%al, %al
	repnz
	scasb

.L_user_strlcpy_source_done:
	// Restore the old fault handler
	movq	%rax, (%r8)

	// Calculate total string length and return.
	movq	%rsi, %rax
	subq	%r9, %rax
	dec		%rax
	ret

.L_user_strlcpy_error:
	// Restore the old fault handler. Return a generic error, the wrapper
	// routine will deal with it.
	movq	%rax, (%r8)
	movq	$-1, %rax
	ret
FUNCTION_END(arch_cpu_user_strlcpy)
