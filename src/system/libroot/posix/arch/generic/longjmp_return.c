/* 
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>. All rights
 * reserved. Distributed under the terms of the Haiku License.
 */

#include <setjmp.h>

/**	This function is called by [sig]longjmp(). The caller has already
	set up the registers and stack, so that returning from the function
	will return to the place where [sig]setjmp() was invoked. It resets
	the signal mask and validates the value supplied to [sig]longjmp().
 */

int __longjmp_return(jmp_buf buffer, int value);

int
__longjmp_return(jmp_buf buffer, int value)
{
	if (buffer[0].mask_was_saved)
		sigprocmask(SIG_SETMASK, &buffer[0].saved_mask, NULL);

	return (value == 0 ? 1 : value);
}
