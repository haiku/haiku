/* 
 * Copyright 2003, Ingo Weinhold, bonefish@users.sf.net. All rights reserved.
 * Distributed under the terms of the MIT License.
*/

#define FUNCTION(x) .global x; .type x,@function; x

/* void *get_stack_frame() */
FUNCTION(get_stack_frame):
	mov		%ebp,%eax
	ret
