/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#define FUNCTION(x) .global x; .type x,@function; x


/* uint64 rdtsc() */
FUNCTION(rdtsc):
	rdtsc
	/* Convert to 64-bit result in rax. */
	shlq    $32, %rdx
	orq     %rdx, %rax
	ret

FUNCTION(execute_n_instructions):
	movl	%edi, %ecx
	shrl	$4, %ecx
.again:
	xorl	%eax, %eax
	xorl	%eax, %eax
	xorl	%eax, %eax
	xorl	%eax, %eax
	xorl	%eax, %eax
	xorl	%eax, %eax
	xorl	%eax, %eax
	xorl	%eax, %eax
	xorl	%eax, %eax
	xorl	%eax, %eax
	xorl	%eax, %eax
	xorl	%eax, %eax
	xorl	%eax, %eax
	xorl	%eax, %eax
	xorl	%eax, %eax
	xorl	%eax, %eax
	loop	.again
	ret
