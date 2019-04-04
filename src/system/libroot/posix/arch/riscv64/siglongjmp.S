/* 
 * Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>


/* int __siglongjmp(jmp_buf buffer, int value) */
FUNCTION(siglongjmp):
FUNCTION(longjmp):
FUNCTION(_longjmp):
	ret
FUNCTION_END(siglongjmp)
FUNCTION_END(longjmp)
FUNCTION_END(_longjmp)

#pragma weak longjmp=siglongjmp

