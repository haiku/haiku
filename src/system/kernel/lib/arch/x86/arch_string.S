/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
*/

#if !_BOOT_MODE
#	include "asm_offsets.h"
#endif

#include <asm_defs.h>


// We don't need the indirection in the boot loader.
#if _BOOT_MODE
#	define memcpy_generic	memcpy
#	define memset_generic	memset
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
	// TODO: The addresses might not be aligned!
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
FUNCTION_END(memcpy_generic)
SYMBOL(memcpy_generic_end):


/* void *memset(void *dest, int value, size_t length); */
.align 4
FUNCTION(memset_generic):
	push	%ebp
	mov		%esp, %ebp

	// %eax, %ecx, and %edx are scratch registers -- we only have to save %edi
	push	%edi

	// get the parameters
	mov		16(%ebp), %ecx
	mov		12(%ebp), %eax
	mov		8(%ebp), %edi

	// When touching less than 12 bytes, we just do it bytewise. We might be
	// able to process one or two lwords lwordwise, but the additional overhead
	// isn't worth it.
	cmp		$12, %ecx
	jl		2f

	// buffer address lword-aligned?
	mov		%edi, %edx
	and		$0x3, %edx
	jz		1f

	// the buffer is unaligned -- copy the first bytes bytewise
	mov		$4, %ecx
	sub		%edx, %ecx
	rep 	stosb

	mov		16(%ebp), %ecx
	sub		$4, %ecx
	add		%edx, %ecx

1:	// lwordwise
	// prepare %eax -- the low byte must be copied to the other bytes
	mov		%al, %ah
	mov		%eax, %edx
	shl		$16, %eax
	mov		%dx, %ax

	// get the unaligned remainder into %edx
	mov		%ecx, %edx
	and		$0x3, %edx

	// write words
	shr		$2, %ecx
	rep		stosl

	mov		%edx, %ecx

2:	// bytewise (remaining bytes)
	rep 	stosb

	pop		%edi

	// return value is the value passed in
	mov		8(%ebp), %eax

	mov		%ebp, %esp
	pop		%ebp
	ret
FUNCTION_END(memset_generic)
SYMBOL(memset_generic_end):


#if !_BOOT_MODE

.align 4
FUNCTION(memcpy):
	jmp		*(gOptimizedFunctions + X86_OPTIMIZED_FUNCTIONS_memcpy)
FUNCTION_END(memcpy)

FUNCTION(memset):
	jmp		*(gOptimizedFunctions + X86_OPTIMIZED_FUNCTIONS_memset)
FUNCTION_END(memset)

#endif	// !_BOOT_MODE
