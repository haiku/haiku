/* 
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <setjmp.h>


/*!	This function is called by [sig]longjmp(). */

int __longjmp_return(jmp_buf buffer, int value);

int
__longjmp_return(jmp_buf buffer, int value)
{
	return (value == 0 ? 1 : value);
}
