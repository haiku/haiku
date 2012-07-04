/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>


.text


/* status_t get_current_cpuid(cpuid_info* info, uint32 eaxRegister) */
FUNCTION(get_current_cpuid):
 	push	%rbx
	movl	%esi, %eax
 	cpuid
 	movl	%eax, 0(%rdi)
 	movl	%ebx, 4(%rdi)
 	movl	%edx, 8(%rdi)
 	movl	%ecx, 12(%rdi)
 	xorl	%eax, %eax
		// B_OK
 	pop		%rbx
 	ret
FUNCTION_END(get_current_cpuid)


/* uint32 get_eflags(void) */
FUNCTION(get_eflags):
	// The top 32 bits of RFLAGS are reserved, we can ignore them.
 	pushf
 	pop		%rax
	mov		%eax, %eax
 	ret
FUNCTION_END(get_eflags)


/* void set_eflags(uint32 val) */
FUNCTION(set_eflags):
	mov		%edi, %edi
 	push	%rdi
 	popf
	ret
FUNCTION_END(set_eflags)
