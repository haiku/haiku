/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <setjmp.h>


/** This function is called by sigsetjmp() only */

int __setjmp_save_sigs(jmp_buf buffer, int saveMask);

int
__setjmp_save_sigs(jmp_buf buffer, int saveMask)
{
	return 0;
}
