/* AMD64 __mpn_submul_1 -- Multiply a limb vector with a limb and subtract
   the result from a second limb vector.
   Copyright (C) 2004 Free Software Foundation, Inc.
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

	.text
ENTRY (__mpn_submul_1)
	movq	%rdx, %r11
	leaq	(%rsi,%r11,8), %rsi
	leaq	(%rdi,%r11,8), %rdi
	negq	%r11
	xorl	%r8d, %r8d
	.p2align 3
L(loop):
	movq	(%rsi,%r11,8), %rax
	movq	(%rdi,%r11,8), %r10
	mulq	%rcx
	subq	%r8, %r10
	movl	$0, %r8d
	adcl	%r8d, %r8d
	subq	%rax, %r10
	adcq	%rdx, %r8
	movq	%r10, (%rdi,%r11,8)
	incq	%r11
	jne	L(loop)
	movq	%r8, %rax
	ret
END (__mpn_submul_1)
