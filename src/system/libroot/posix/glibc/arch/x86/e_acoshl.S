/* ix87 specific implementation of arcsinh.
   Copyright (C) 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1996.

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

#ifdef __ELF__
	.section .rodata
#else
	.text
#endif

	.align ALIGNARG(4)
	/* Please note that we use double value for 1.0.  This number
	   has an exact representation and so we don't get accuracy
	   problems.  The advantage is that the code is simpler.  */
	ASM_TYPE_DIRECTIVE(one,@object)
one:	.double 1.0
	ASM_SIZE_DIRECTIVE(one)
	/* It is not important that this constant is precise.  It is only
	   a value which is known to be on the safe side for using the
	   fyl2xp1 instruction.  */
	ASM_TYPE_DIRECTIVE(limit,@object)
limit:	.double 0.29
	ASM_SIZE_DIRECTIVE(limit)

#ifdef PIC
#define MO(op) op##@GOTOFF(%edx)
#else
#define MO(op) op
#endif

	.text
ENTRY(__ieee754_acoshl)
	movl	12(%esp), %ecx
	andl	$0xffff, %ecx
	cmpl	$0x3fff, %ecx
	jl	5f			// < 1 => invalid
	fldln2				// log(2)
	fldt	4(%esp)			// x : log(2)
	cmpl	$0x4020, %ecx
	ja	3f			// x > 2^34
#ifdef	PIC
	call	1f
1:	popl	%edx
	addl	$_GLOBAL_OFFSET_TABLE_+[.-1b], %edx
#endif
	cmpl	$0x4000, %ecx
	ja	4f			// x > 2

	// 1 <= x <= 2 => y = log1p(x-1+sqrt(2*(x-1)+(x-1)^2))
	fsubl	MO(one)			// x-1 : log(2)
	fld	%st			// x-1 : x-1 : log(2)
	fmul	%st(1)			// (x-1)^2 : x-1 : log(2)
	fadd	%st(1)			// x-1+(x-1)^2 : x-1 : log(2)
	fadd	%st(1)			// 2*(x-1)+(x-1)^2 : x-1 : log(2)
	fsqrt				// sqrt(2*(x-1)+(x-1)^2) : x-1 : log(2)
	faddp				// x-1+sqrt(2*(x-1)+(x-1)^2) : log(2)
	fcoml	MO(limit)
	fnstsw
	sahf
	ja	2f
	fyl2xp1				// log1p(x-1+sqrt(2*(x-1)+(x-1)^2))
	ret

2:	faddl	MO(one)			// x+sqrt(2*(x-1)+(x-1)^2) : log(2)
	fyl2x				// log(x+sqrt(2*(x-1)+(x-1)^2))
	ret

	// x > 2^34 => y = log(x) + log(2)
	.align ALIGNARG(4)
3:	fyl2x				// log(x)
	fldln2				// log(2) : log(x)
	faddp				// log(x)+log(2)
	ret

	// 2^34 > x > 2 => y = log(2*x - 1/(x+sqrt(x*x-1)))
	.align ALIGNARG(4)
4:	fld	%st			// x : x : log(2)
	fadd	%st, %st(1)		// x : 2*x : log(2)
	fld	%st			// x : x : 2*x : log(2)
	fmul	%st(1)			// x^2 : x : 2*x : log(2)
	fsubl	MO(one)			// x^2-1 : x : 2*x : log(2)
	fsqrt				// sqrt(x^2-1) : x : 2*x : log(2)
	faddp				// x+sqrt(x^2-1) : 2*x : log(2)
	fdivrl	MO(one)			// 1/(x+sqrt(x^2-1)) : 2*x : log(2)
	fsubrp				// 2*x+1/(x+sqrt(x^2)-1) : log(2)
	fyl2x				// log(2*x+1/(x+sqrt(x^2-1)))
	ret

	// x < 1 => NaN
	.align ALIGNARG(4)
5:	fldz
	fdiv	%st, %st(0)
	ret
END(__ieee754_acoshl)
