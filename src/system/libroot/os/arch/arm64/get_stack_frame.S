/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#include <asm_defs.h>

/* void* get_stack_frame(void) */
FUNCTION(get_stack_frame):
	mov		x0, fp
	ret
FUNCTION_END(get_stack_frame)
