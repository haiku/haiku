/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
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
	mov		4(%esp), %eax

	// fill __jmp_buf structure with current registers (%ecx can be clobbered)
	mov		%ebx, JMP_REGS_EBX(%eax)
	mov		%esi, JMP_REGS_ESI(%eax)
	mov		%edi, JMP_REGS_EDI(%eax)

	// save stack and return address (because that's where we intend to jump to)
	lea		4(%esp,1), %ecx
	mov		%ecx, JMP_REGS_ESP(%eax)
	mov		0(%esp,1), %ecx
	mov		%ecx, JMP_REGS_PC(%eax)

	jmp		__setjmp_save_sigs
