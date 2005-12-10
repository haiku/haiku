/* 
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>. All rights
 * reserved. Distributed under the terms of the Haiku License.
 */

#include "setjmp_internal.h"

/* int setjmp(jmp_buf buffer) */
FUNCTION(setjmp):
	// call __sigsetjmp with saveMask = 0
	addi	%r4, 0, 0
	b		__sigsetjmp

#pragma weak _setjmp=setjmp

