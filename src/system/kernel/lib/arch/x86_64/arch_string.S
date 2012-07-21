/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>

#include "asm_offsets.h"


.align 8
FUNCTION(memcpy_generic):
	push	%rbp
	movq	%rsp, %rbp

	// Preserve original destination address for return value.
	movq	%rdi, %rax

	// size -> %rcx
	movq	%rdx, %rcx

	// For small copies, always do it bytewise, the additional overhead is
	// not worth it.
	cmp		$24, %rcx
	jl		.Lmemcpy_generic_byte_copy

	// Do both source and dest have the same alignment?
	movq	%rsi, %r8
	xorq	%rdi, %r8
	test	$7, %r8
	jnz		.Lmemcpy_generic_byte_copy

	// Align up to an 8-byte boundary.
	movq	%rdi, %r8
	andq	$7, %r8
	jz		.Lmemcpy_generic_qword_copy
	movq	$8, %rcx
	subq	%r8, %rcx
	subq	%rcx, %rdx			// Subtract from the overall count.
	rep
	movsb

	// Get back the original count value.
	movq	%rdx, %rcx
.Lmemcpy_generic_qword_copy:
	// Move by quadwords.
	shrq	$3, %rcx
	rep
	movsq

	// Get the remaining count.
	movq	%rdx, %rcx
	andq	$7, %rcx
.Lmemcpy_generic_byte_copy:
	// Move any remaining data by bytes.
	rep
	movsb

	pop		%rbp
	ret
FUNCTION_END(memcpy_generic)
SYMBOL(memcpy_generic_end):


.align 8
FUNCTION(memset_generic):
	push	%rbp
	movq	%rsp, %rbp

	// Preserve original destination address for return value.
	movq	%rdi, %r8

	// size -> %rcx, value -> %al
	movq	%rdx, %rcx
	movl	%esi, %eax

	// Move by bytes.
	rep
	stosb

	movq	%r8, %rax
	pop		%rbp
	ret
FUNCTION_END(memset_generic)
SYMBOL(memset_generic_end):


FUNCTION(memcpy):
	jmp		*(gOptimizedFunctions + X86_OPTIMIZED_FUNCTIONS_memcpy)
FUNCTION_END(memcpy)

FUNCTION(memset):
	jmp		*(gOptimizedFunctions + X86_OPTIMIZED_FUNCTIONS_memset)
FUNCTION_END(memset)

