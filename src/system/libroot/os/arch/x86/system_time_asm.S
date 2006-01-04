/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#define FUNCTION(x) .global x; .type x,@function; x

.text

/* saves the conversion factor needed for system_time */
cv_factor:
	.word 0

FUNCTION(__x86_setup_system_time):
	movl	4(%esp),%eax
	movl	%eax,cv_factor
	ret

/* long long system_time(); */
FUNCTION(system_time):
	/* load 64-bit factor into %eax (low), %edx (high) */
	rdtsc		/* time in %edx,%eax */

	pushl	%ebx
	pushl	%ecx
	movl	cv_factor, %ebx
	movl	%edx, %ecx	/* save high half */
	mull	%ebx 		/* truncate %eax, but keep %edx */
	movl	%ecx, %eax
	movl	%edx, %ecx	/* save high half of low */
	mull	%ebx /*, %eax*/
	/* now compute  [%edx, %eax] + [%ecx], propagating carry */
	subl	%ebx, %ebx	/* need zero to propagate carry */
	addl	%ecx, %eax
	adc		%ebx, %edx
	popl	%ecx
	popl	%ebx
	ret
