/*
 * Copyright 2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <asm_defs.h>

/* int sigsetjmp(jmp_buf buffer, int saveMask) */
FUNCTION(__sigsetjmp):
FUNCTION(sigsetjmp):
	b	.
FUNCTION_END(sigsetjmp)
FUNCTION_END(__sigsetjmp)


/* int setjmp(jmp_buf buffer) */
FUNCTION(setjmp):
FUNCTION(_setjmp):
	b	.
FUNCTION_END(setjmp)

#pragma weak _setjmp=setjmp
