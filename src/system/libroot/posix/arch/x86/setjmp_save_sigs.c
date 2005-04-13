/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <setjmp.h>


/** This function is called by sigsetjmp() only */

int __setjmp_save_sigs(jmp_buf buffer, int saveMask);

int
__setjmp_save_sigs(jmp_buf buffer, int saveMask)
{
	buffer[0].mask_was_saved = saveMask && sigprocmask(SIG_BLOCK, NULL, &buffer[0].saved_mask) == 0;
		// only set mask_was_saved if sigprocmask() was successful

	return 0;
}
