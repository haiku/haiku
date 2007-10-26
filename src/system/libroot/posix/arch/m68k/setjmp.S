/* 
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>. All rights
 * reserved. Distributed under the terms of the Haiku License.
 */

#include "setjmp_internal.h"

/* int setjmp(jmp_buf buffer) */
FUNCTION(setjmp):
	move.l	(%a7)+,%a0
	move.l	(%a7)+,%d0
	clr.l	-(%a7)		/* push 0 as 2nd arg */
	move.l	%d0,-(%a7)
	move.l	%a0,-(%a7)
	// call __sigsetjmp with saveMask = 0
	jmp	__sigsetjmp


#pragma weak _setjmp=setjmp

