/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#if !_BOOT_MODE
#	include "asm_offsets.h"
#endif

#include <asm_defs.h>


// We don't need the indirection in the boot loader.
#if _BOOT_MODE
#	define memcpy_generic	memcpy
#endif


.align 4
FUNCTION(memcpy_generic):
	pushl	%esi
	pushl	%edi
	movl	12(%esp),%edi	/* dest */
	movl	%edi,%eax	/* save dest ptr as return address */
	movl	16(%esp),%esi	/* source */
	movl	20(%esp),%ecx	/* count */

	/* move by words */
	cld
	shrl	$2,%ecx
	rep
	movsl

	/* move any remaining data by bytes */
	movl	20(%esp),%ecx
	andl	$3,%ecx
	rep
	movsb

	popl	%edi
	popl	%esi
	ret
SYMBOL(memcpy_generic_end):


#if !_BOOT_MODE

.align 4
FUNCTION(memcpy):
	jmp		*(gOptimizedFunctions + X86_OPTIMIZED_FUNCTIONS_memcpy)

#endif	// !_BOOT_MODE
