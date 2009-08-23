/* 
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * Copyright 2005 Ingo Weinhold, bonefish@cs.tu-berlin.de
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "setjmp_internal.h"

/* int __siglongjmp(jmp_buf buffer, int value) */
FUNCTION(siglongjmp):
FUNCTION(longjmp):
FUNCTION(_longjmp):
	// restore non-volatile general purpose registers
	nop

	// restore special registers (link, condition)
	nop

#pragma weak longjmp=siglongjmp

