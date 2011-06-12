/*
 * Copyright 2005-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <setjmp.h>


/*!	This function is called by [sig]longjmp(). The caller has already
	set up the registers and stack, so that returning from the function
	will return to the place where [sig]setjmp() was invoked. It resets
	the signal mask and validates the value supplied to [sig]longjmp().
 */
int __longjmp_return(jmp_buf buffer, int value);

int
__longjmp_return(jmp_buf buffer, int value)
{
	if (buffer[0].inverted_signal_mask != 0) {
		sigset_t signalMask = ~buffer[0].inverted_signal_mask;
		sigprocmask(SIG_SETMASK, &signalMask, NULL);
	}

	return (value == 0 ? 1 : value);
}
