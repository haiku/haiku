/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#define FUNCTION(x) .global x; .type x,@function; x

.align 4
FUNCTION(memcpy):
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
