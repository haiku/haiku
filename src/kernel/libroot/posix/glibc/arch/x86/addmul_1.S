/* Pentium __mpn_addmul_1 -- Multiply a limb vector with a limb and add
   the result to a second limb vector.
   Copyright (C) 1992, 94, 96, 97, 98, 00 Free Software Foundation, Inc.
   This file is part of the GNU MP Library.

   The GNU MP Library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2.1 of the License, or (at your
   option) any later version.

   The GNU MP Library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
   License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with the GNU MP Library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA. */

#include "sysdep.h"
#include "asm-syntax.h"
#include "bp-sym.h"
#include "bp-asm.h"

#define PARMS	LINKAGE+16	/* space for 4 saved regs */
#define RES	PARMS
#define S1	RES+PTR_SIZE
#define SIZE	S1+PTR_SIZE
#define S2LIMB	SIZE+4

#define res_ptr edi
#define s1_ptr esi
#define size ecx
#define s2_limb ebx

	.text
ENTRY (BP_SYM (__mpn_addmul_1))
	ENTER

	pushl	%edi
	pushl	%esi
	pushl	%ebp
	pushl	%ebx

	movl	RES(%esp), %res_ptr
	movl	S1(%esp), %s1_ptr
	movl	SIZE(%esp), %size
	movl	S2LIMB(%esp), %s2_limb
#if __BOUNDED_POINTERS__
	shll	$2, %size	/* convert limbs to bytes */
	CHECK_BOUNDS_BOTH_WIDE (%res_ptr, RES(%esp), %size)
	CHECK_BOUNDS_BOTH_WIDE (%s1_ptr, S1(%esp), %size)
	shrl	$2, %size
#endif
	leal	(%res_ptr,%size,4), %res_ptr
	leal	(%s1_ptr,%size,4), %s1_ptr
	negl	%size
	xorl	%ebp, %ebp
	ALIGN (3)

L(oop):	adcl	$0, %ebp
	movl	(%s1_ptr,%size,4), %eax

	mull	%s2_limb

	addl	%ebp, %eax
	movl	(%res_ptr,%size,4), %ebp

	adcl	$0, %edx
	addl	%eax, %ebp

	movl	%ebp, (%res_ptr,%size,4)
	incl	%size

	movl	%edx, %ebp
	jnz	L(oop)

	adcl	$0, %ebp
	movl	%ebp, %eax
	popl	%ebx
	popl	%ebp
	popl	%esi
	popl	%edi

	LEAVE
	ret
#undef size
END (BP_SYM (__mpn_addmul_1))
