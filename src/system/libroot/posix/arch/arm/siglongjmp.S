/* 
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>. All rights
 * reserved. Distributed under the terms of the Haiku License.
 */

#include <asm_defs.h>

//#include "setjmp_internal.h"

/* int __siglongjmp(jmp_buf buffer, int value) */
FUNCTION(siglongjmp):
FUNCTION(longjmp):
FUNCTION(_longjmp):
	str	r1,[r0]
        ldmia	r0,{r0-r14}
	bx	lr
//	bl	__longjmp_return
FUNCTION_END(siglongjmp)
FUNCTION_END(longjmp)
FUNCTION_END(_longjmp)

#pragma weak longjmp=siglongjmp
