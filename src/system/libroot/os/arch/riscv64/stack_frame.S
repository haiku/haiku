/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>


/* void* get_stack_frame(void) */
FUNCTION(get_stack_frame):
	mv a0, fp
	ret
FUNCTION_END(get_stack_frame)
