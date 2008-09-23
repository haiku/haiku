/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net. All rights reserved.
 * Distributed under the terms of the MIT License.
*/

#include <asm_defs.h>


/* void *get_stack_frame() */
FUNCTION(get_stack_frame):
	mov		%ebp, %eax
	ret
FUNCTION_END(get_stack_frame)
