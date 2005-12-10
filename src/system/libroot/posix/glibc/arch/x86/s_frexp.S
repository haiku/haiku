/* ix87 specific frexp implementation for double.
   Copyright (C) 1997, 2000 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1997.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <machine/asm.h>
#include "bp-sym.h"
#include "bp-asm.h"

#ifdef __ELF__
	.section .rodata
#else
	.text
#endif

	.align ALIGNARG(4)
	ASM_TYPE_DIRECTIVE(two54,@object)
two54:	.byte 0, 0, 0, 0, 0, 0, 0x50, 0x43
	ASM_SIZE_DIRECTIVE(two54)

#ifdef PIC
#define MO(op) op##@GOTOFF(%edx)
#else
#define MO(op) op
#endif

#define PARMS	LINKAGE		/* no space for saved regs */
#define VAL0	PARMS
#define VAL1	VAL0+4
#define EXPP	VAL1+4

	.text
ENTRY (BP_SYM (__frexp))
	ENTER

	movl	VAL0(%esp), %ecx
	movl	VAL1(%esp), %eax
	movl	%eax, %edx
	andl	$0x7fffffff, %eax
	orl	%eax, %ecx
	jz	1f
	xorl	%ecx, %ecx
	cmpl	$0x7ff00000, %eax
	jae	1f

	cmpl	$0x00100000, %eax
	jae	2f

	fldl	VAL0(%esp)
#ifdef	PIC
	call	3f
3:	popl	%edx
	addl	$_GLOBAL_OFFSET_TABLE_+[.-3b], %edx
#endif
	fmull	MO(two54)
	movl	$-54, %ecx
	fstpl	VAL0(%esp)
	fwait
	movl	VAL1(%esp), %eax
	movl	%eax, %edx
	andl	$0x7fffffff, %eax

2:	shrl	$20, %eax
	andl	$0x800fffff, %edx
	subl	$1022, %eax
	orl	$0x3fe00000, %edx
	addl	%eax, %ecx
	movl	%edx, VAL1(%esp)

	/* Store %ecx in the variable pointed to by the second argument,
	   get the factor from the stack and return.  */
1:	movl	EXPP(%esp), %eax
	CHECK_BOUNDS_BOTH_WIDE (%eax, EXPP(%esp), $4)
	fldl	VAL0(%esp)
	movl	%ecx, (%eax)

	LEAVE
	ret
END (BP_SYM (__frexp))
weak_alias (BP_SYM (__frexp), BP_SYM (frexp))
