/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <asm_defs.h>
#include <commpage_defs.h>


FUNCTION(memcpy):
	movq	__gCommPageAddress@GOTPCREL(%rip), %rax
	movq	(%rax), %rax
	addq	8 * COMMPAGE_ENTRY_X86_MEMCPY(%rax), %rax
	jmp 	*%rax
FUNCTION_END(memcpy)

FUNCTION(memset):
	movq	__gCommPageAddress@GOTPCREL(%rip), %rax
	movq	(%rax), %rax
	addq	8 * COMMPAGE_ENTRY_X86_MEMSET(%rax), %rax
	jmp 	*%rax
FUNCTION_END(memset)
