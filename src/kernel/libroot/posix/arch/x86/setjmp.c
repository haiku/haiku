/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <setjmp.h>


int
setjmp(jmp_buf buffer)
{
	return sigsetjmp(buffer, 0);
}

#pragma weak _setjmp = setjmp
