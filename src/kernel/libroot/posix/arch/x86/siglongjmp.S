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
	mov		4(%esp,1), %ecx
	mov		8(%esp,1), %eax

	// restore registers
	mov		JMP_REGS_EBX(%ecx), %ebx
	mov		JMP_REGS_ESI(%ecx), %esi
	mov		JMP_REGS_EDI(%ecx), %edi
	mov		JMP_REGS_EBP(%ecx), %ebp
	mov		JMP_REGS_ESP(%ecx), %esp

	// jump back to the old program location
	mov		JMP_REGS_PC(%ecx), %edx
	jmp		*%edx

#pragma weak longjmp=siglongjmp
