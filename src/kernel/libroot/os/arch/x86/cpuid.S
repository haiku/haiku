/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#define FUNCTION(x) .global x; .type x,@function; x

.text

/* void cpuid(unsigned int eax_value, unsigned int *return_data) */
FUNCTION(cpuid):
 	pushl	%ebx
 	pushl	%edi
 	movl	12(%esp),%eax	/* first arg sets up eax */
 	movl	16(%esp),%edi	/* second arg points to the return area */
 	cpuid
 	movl	%eax,0(%edi)	/* copy the regs into the return area */
 	movl	%ebx,4(%edi)	/* ... */
 	movl	%ecx,8(%edi)	/* ... */
 	movl	%edx,12(%edi)	/* ... */
 	popl	%edi
 	popl	%ebx
 	ret


/* unsigned int get_eflags(void) */
FUNCTION(get_eflags):
 	pushfl
 	popl	%eax
 	ret


/* void set_eflags(unsigned int val) */
FUNCTION(set_eflags):
 	pushl	4(%esp)
 	popfl
	ret
