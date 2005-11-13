/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "setjmp_internal.h"


/* int setjmp(jmp_buf buffer) */
FUNCTION(setjmp):
	pop		%eax		// pop argument and return pointer from stack
	pop		%ecx
	pushl	$0			// push new argument (for saveMask) and old stuff
	push	%ecx		// back onto the stack
	push	%eax
	jmp		__sigsetjmp	// let sigsetjmp() do the actual work

#pragma weak _setjmp=setjmp

