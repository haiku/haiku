/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>


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
