/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/

#include "setjmp_internal.h"


/**	This is a BeOS compatible siglongjmp() implementation;
 *	longjmp() and _longjmp() are both calling this function.
 */


/* int __siglongjmp(jmp_buf buffer, int value) */
FUNCTION(siglongjmp):
FUNCTION(longjmp):
FUNCTION(_longjmp):
	mov		4(%esp), %ecx
	mov		8(%esp), %eax

	// restore registers
	mov		JMP_REGS_EBX(%ecx), %ebx
	mov		JMP_REGS_ESI(%ecx), %esi
	mov		JMP_REGS_EDI(%ecx), %edi
	mov		JMP_REGS_EBP(%ecx), %ebp
	mov		JMP_REGS_ESP(%ecx), %esp

	// prepare the stack so that we will return to the setjmp() program location
	mov		JMP_REGS_PC(%ecx), %edx
	push	%edx				// return address

	// let __setjmp_save_sigs deal with the signal mask and the return value
	push	%eax				// value
	push	%ecx				// buffer
	call	__longjmp_return
	add		$8, %esp

	ret
FUNCTION_END(siglongjmp)


#pragma weak longjmp=siglongjmp
