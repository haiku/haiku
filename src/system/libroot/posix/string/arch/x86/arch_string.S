/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#if !_KERNEL_MODE
	// TODO: This should not even be compiled for the kernel. Fix the TODO in
	// src/system/kernel/lib/Jamfile!

#include <commpage.h>

#define FUNCTION(x) .global x; .type x,@function; x

.align 4
FUNCTION(memcpy):
	jmp	*(USER_COMMPAGE_ADDR + COMMPAGE_ENTRY_X86_MEMCPY * 4)

#endif	// !_KERNEL_MODE
