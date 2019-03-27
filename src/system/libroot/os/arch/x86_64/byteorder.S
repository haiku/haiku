/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>


/* uint16 __swap_int16(uint16 value) */
FUNCTION(__swap_int16):
	movl	%edi, %eax
	bswap	%eax
	shr		$16, %eax
	ret
FUNCTION_END(__swap_int16)

/* uint32 __swap_int32(uint32 value) */
FUNCTION(__swap_int32):
	movl	%edi, %eax
	bswap	%eax
	ret
FUNCTION_END(__swap_int32)

/* uint64 __swap_int64(uint64 value) */
FUNCTION(__swap_int64):
	movq	%rdi, %rax
	bswap	%rax
	ret
FUNCTION_END(__swap_int64)

/* float __swap_float(float value) */
FUNCTION(__swap_float):
	sub		$8, %rsp
	movss	%xmm0, (%rsp)
	movl	(%rsp), %eax
	bswap	%eax
	movl	%eax, (%rsp)
	movss	(%rsp), %xmm0
	add		$8, %rsp
	ret
FUNCTION_END(__swap_float)

/* double __swap_double(double value) */
FUNCTION(__swap_double):
	sub		$8, %rsp
	movsd	%xmm0, (%rsp)
	movq	(%rsp), %rax
	bswap	%rax
	movq	%rax, (%rsp)
	movsd	(%rsp), %xmm0
	add		$8, %rsp
	ret
FUNCTION_END(__swap_double)
