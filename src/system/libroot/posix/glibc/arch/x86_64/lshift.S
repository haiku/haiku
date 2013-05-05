/* AMD64 __mpn_lshift --
   Copyright 2004, 2006 Free Software Foundation, Inc.
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
ENTRY (__mpn_lshift)
	movq	-8(%rsi,%rdx,8), %mm7
	movd	%ecx, %mm1
	movl	$64, %eax
	subl	%ecx, %eax
	movd	%eax, %mm0
	movq	%mm7, %mm3
	psrlq	%mm0, %mm7
	movd	%mm7, %rax
	subq	$2, %rdx
	jl	L(endo)
	.p2align 2
L(loop):
	movq	(%rsi,%rdx,8), %mm6
	movq	%mm6, %mm2
	psrlq	%mm0, %mm6
	psllq	%mm1, %mm3
	por	%mm6, %mm3
	movq	%mm3, 8(%rdi,%rdx,8)
	je	L(ende)
	movq	-8(%rsi,%rdx,8), %mm7
	movq	%mm7, %mm3
	psrlq	%mm0, %mm7
	psllq	%mm1, %mm2
	por	%mm7, %mm2
	movq	%mm2, (%rdi,%rdx,8)
	subq	$2, %rdx
	jge	L(loop)
L(endo):
	movq	%mm3, %mm2
L(ende):
	psllq	%mm1, %mm2
	movq	%mm2, (%rdi)
	emms
	ret
END (__mpn_lshift)
