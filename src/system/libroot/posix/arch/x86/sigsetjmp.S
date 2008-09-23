/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "setjmp_internal.h"


/**	This is a BeOS compatible __sigsetjmp() implementation; there,
 *	setjmp() and sigsetjmp() are both macros to this function.
 *
 *	It first saves the register/stack environment of the running thread,
 *	and then calls __setjmp_save_sigs() which will save the signal state
 *	if it was asked to do this.
 */

/* int sigsetjmp(jmp_buf buffer, int saveMask) */
FUNCTION(__sigsetjmp):
FUNCTION(sigsetjmp):
	// return address to %edx, stack pointer for return to %ecx (both are
	// scratch registers)
	mov		0(%esp), %edx
	lea		4(%esp), %ecx

	// buffer to %eax
	mov		4(%esp), %eax

sigsetjmp_setjmp_entry:
	// fill __jmp_buf structure with current registers
	mov		%ebx, JMP_REGS_EBX(%eax)
	mov		%esi, JMP_REGS_ESI(%eax)
	mov		%edi, JMP_REGS_EDI(%eax)
	mov		%ebp, JMP_REGS_EBP(%eax)

	// save stack and return address (because that's where we intend to jump to)
	mov		%ecx, JMP_REGS_ESP(%eax)
	mov		%edx, JMP_REGS_PC(%eax)

	jmp		__setjmp_save_sigs
FUNCTION_END(sigsetjmp)


/* int setjmp(jmp_buf buffer) */
FUNCTION(setjmp):
	// prepare %edx, %ecx, and %eax for sigsetjmp
	mov		0(%esp), %edx
	lea		4(%esp), %ecx
	mov		(%ecx), %eax

	// let sigsetjmp do the real work
	pushl	$0							// saveMask
	push	%eax						// buffer
	call	sigsetjmp_setjmp_entry
	add		$8, %esp

	ret
FUNCTION_END(setjmp)


#pragma weak _setjmp=setjmp
