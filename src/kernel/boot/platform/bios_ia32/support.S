/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#define FUNCTION(x) .global x; .type x,@function; x


/* uint64 rdtsc() */
FUNCTION(rdtsc):
	rdtsc
	ret

FUNCTION(execute_n_instructions):
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

FUNCTION(system_time):
   	/* load 64-bit factor into %eax (low), %edx (high) */
   	/* hand-assemble rdtsc -- read time stamp counter */
	rdtsc		/* time in %edx,%eax */

	pushl	%ebx
	movl	gTimeConversionFactor, %ebx
	movl	%edx, %ecx	/* save high half */
	mull	%ebx 		/* truncate %eax, but keep %edx */
	movl	%ecx, %eax
	movl	%edx, %ecx	/* save high half of low */
	mull	%ebx			/*, %eax*/
	/* now compute  [%edx, %eax] + [%ecx], propagating carry */
	subl	%ebx, %ebx	/* need zero to propagate carry */
	addl	%ecx, %eax
	adc		%ebx, %edx
	popl	%ebx
	ret

FUNCTION(cpuid):
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

FUNCTION(get_eflags):
 	pushfl
 	popl	%eax
 	ret

FUNCTION(set_eflags):
 	pushl	4(%esp)
 	popfl
	ret

null_idt_descr:
	.word	0
	.word	0,0
		
FUNCTION(reboot):
	lidt	null_idt_descr
	int		$0
done:
	jmp		done

