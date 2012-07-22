/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
*/


#include <asm_defs.h>


/* void* get_stack_frame(void) */
FUNCTION(get_stack_frame):
	movq	%rbp, %rax
	ret
FUNCTION_END(get_stack_frame)


/* void* __arch_get_caller(void); */
FUNCTION(__arch_get_caller):
	movq	8(%rbp), %rax
	ret
FUNCTION_END(__arch_get_caller)

