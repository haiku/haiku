/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <setjmp.h>

#ifdef COMPILE_FOR_R5
#	undef setjmp
#endif


int
setjmp(jmp_buf buffer)
{
	return sigsetjmp(buffer, 0);
}

#pragma weak _setjmp = setjmp
