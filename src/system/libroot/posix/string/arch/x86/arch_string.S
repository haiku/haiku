/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <asm_defs.h>
#include <commpage_defs.h>


.align 4

FUNCTION(memcpy):
	movl	__gCommPageAddress, %eax
	addl	4 * COMMPAGE_ENTRY_X86_MEMCPY(%eax), %eax
	jmp 	*%eax
FUNCTION_END(memcpy)

FUNCTION(memset):
	movl	__gCommPageAddress, %eax
	addl	4 * COMMPAGE_ENTRY_X86_MEMSET(%eax), %eax
	jmp 	*%eax
FUNCTION_END(memset)
