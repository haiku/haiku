/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the Haiku License.
 */


#include <setjmp.h>


/** This function is called by sigsetjmp() only */

int __setjmp_save_sigs(jmp_buf buffer, int saveMask);

int
__setjmp_save_sigs(jmp_buf buffer, int saveMask)
{
	// If the signal mask shall be saved, we save the inverted signal mask. The
	// reason for this is that due to unblockable signals the inverted signal
	// mask is never zero and thus we can use a zero value to indicate that the
	// mask has not been saved.
	sigset_t signalMask;
	if (saveMask != 0 && sigprocmask(SIG_BLOCK, NULL, &signalMask) == 0)
		buffer[0].inverted_signal_mask = ~signalMask;
	else
		buffer[0].inverted_signal_mask = 0;

	return 0;
}
