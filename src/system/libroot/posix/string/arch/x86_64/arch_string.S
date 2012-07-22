/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <asm_defs.h>
#include <commpage_defs.h>


FUNCTION(memcpy):
	jmp	*(USER_COMMPAGE_ADDR + COMMPAGE_ENTRY_X86_MEMCPY * 8)
FUNCTION_END(memcpy)


FUNCTION(memset):
	jmp	*(USER_COMMPAGE_ADDR + COMMPAGE_ENTRY_X86_MEMSET * 8)
FUNCTION_END(memset)
