/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2018, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <asm_defs.h>


.align 4
FUNCTION(memcpy):
	pushl	%esi
	pushl	%edi
	movl	12(%esp),%edi	/* dest */
	movl	%edi,%eax	/* save dest ptr as return address */
	movl	16(%esp),%esi	/* source */
	movl	20(%esp),%ecx	/* count */

	/* (count == 0 || dest == src) -> quick way out */
	testl	%ecx, %ecx
	je		.tail
	cmpl	%edi, %esi
	je		.tail

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

.tail:
	popl	%edi
	popl	%esi
	ret
FUNCTION_END(memcpy)
SYMBOL(memcpy_end):


/* void *memset(void *dest, int value, size_t length); */
.align 4
FUNCTION(memset):
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
FUNCTION_END(memset)
SYMBOL(memset_end):

