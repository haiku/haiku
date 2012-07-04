/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include <asm_defs.h>


.text

/* void get_current_cpuid(cpuid_info *info, uint32 eaxRegister) */
FUNCTION(get_current_cpuid):
 	pushl	%ebx
 	pushl	%edi
 	movl	12(%esp),%edi	/* first arg points to the cpuid_info structure */
 	movl	16(%esp),%eax	/* second arg sets up eax */
 	cpuid
 	movl	%eax,0(%edi)	/* copy the regs into the cpuid_info structure */
 	movl	%ebx,4(%edi)
 	movl	%edx,8(%edi)
 	movl	%ecx,12(%edi)
 	popl	%edi
 	popl	%ebx
 	xorl	%eax, %eax		/* return B_OK */
 	ret
FUNCTION_END(get_current_cpuid)


/* unsigned int get_eflags(void) */
FUNCTION(get_eflags):
 	pushfl
 	popl	%eax
 	ret
FUNCTION_END(get_eflags)


/* void set_eflags(unsigned int val) */
FUNCTION(set_eflags):
 	pushl	4(%esp)
 	popfl
	ret
FUNCTION_END(set_eflags)
