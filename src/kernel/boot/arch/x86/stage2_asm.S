/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
/* long long rdtsc() */
.global rdtsc
rdtsc:
	rdtsc
	ret

.global execute_n_instructions
execute_n_instructions:
	movl	4(%esp), %ecx
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

.global system_time_setup
system_time_setup:
	/* First divide 1M * 2^32 by proc_clock */
	movl	$0x0F4240, %ecx
	movl	%ecx, %edx
	subl	%eax, %eax
	movl	4(%esp), %ebx
	divl	%ebx, %eax		/* should be 64 / 32 */
	movl	%eax, cv_factor
	ret

.global system_time
system_time:
   	/* load 64-bit factor into %eax (low), %edx (high) */
   	/* hand-assemble rdtsc -- read time stamp counter */
	rdtsc		/* time in %edx,%eax */

	pushl	%ebx
	pushl	%ecx
	movl	cv_factor, %ebx
	movl	%edx, %ecx	/* save high half */
	mull	%ebx 		/* truncate %eax, but keep %edx */
	movl	%ecx, %eax
	movl	%edx, %ecx	/* save high half of low */
	mull	%ebx			/*, %eax*/
	/* now compute  [%edx, %eax] + [%ecx], propagating carry */
	subl	%ebx, %ebx	/* need zero to propagate carry */
	addl	%ecx, %eax
	adc		%ebx, %edx
	popl	%ecx
	popl	%ebx
	ret

.global cpuid
cpuid:
 	pushl	%ebx
 	pushl	%edi
 	movl	12(%esp),%eax
 	movl	16(%esp),%edi
 	cpuid
 	movl	%eax,0(%edi)
 	movl	%ebx,4(%edi)
 	movl	%ecx,8(%edi)
 	movl	%edx,12(%edi)
 	popl	%edi
 	popl	%ebx
 	ret

.global get_eflags
get_eflags:
 	pushfl
 	popl	%eax
 	ret

.global set_eflags
set_eflags:
 	pushl	4(%esp)
 	popfl
	ret


