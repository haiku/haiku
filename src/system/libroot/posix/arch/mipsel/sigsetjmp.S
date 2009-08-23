/* 
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * Copyright 2005 Ingo Weinhold, bonefish@cs.tu-berlin.de
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <asm_defs.h>

#include "setjmp_internal.h"

/* int sigsetjmp(jmp_buf buffer, int saveMask) */
FUNCTION(__sigsetjmp):
FUNCTION(sigsetjmp):
		nop
FUNCTION_END(__sigsetjmp)
FUNCTION_END(sigsetjmp)


/* int setjmp(jmp_buf buffer) */
FUNCTION(setjmp):
		nop
FUNCTION_END(setjmp)

#pragma weak _setjmp=setjmp
