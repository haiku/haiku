/*
 * Copyright 2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>


/* int __siglongjmp(jmp_buf buffer, int value) */
FUNCTION(siglongjmp):
FUNCTION(longjmp):
FUNCTION(_longjmp):
	ldp x19, x20, [x0], #16
	ldp x21, x22, [x0], #16
	ldp x23, x24, [x0], #16
	ldp x25, x26, [x0], #16
	ldp x27, x28, [x0], #16
	ldp x29, x30, [x0], #16

	ldr x2, [x0]
	mov sp, x2

	b __longjmp_return
FUNCTION_END(siglongjmp)
FUNCTION_END(longjmp)
FUNCTION_END(_longjmp)

#pragma weak longjmp=siglongjmp
