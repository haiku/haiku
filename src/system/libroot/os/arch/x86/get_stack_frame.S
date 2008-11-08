/*
 * Copyright 2005-2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
*/

#include <asm_defs.h>


/* void* get_stack_frame(void) */
FUNCTION(get_stack_frame):
	mov		%ebp, %eax
	ret
FUNCTION_END(get_stack_frame)


/* void* __arch_get_caller(void); */
FUNCTION(__arch_get_caller):
	mov		4(%ebp), %eax
	ret
FUNCTION_END(__arch_get_caller)

