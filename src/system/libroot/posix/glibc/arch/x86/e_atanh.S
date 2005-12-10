/* ix87 specific implementation of arctanh function.
   Copyright (C) 1996, 1999 Free Software Foundation, Inc.
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
	ASM_TYPE_DIRECTIVE(half,@object)
half:	.double 0.5
	ASM_SIZE_DIRECTIVE(half)
	ASM_TYPE_DIRECTIVE(one,@object)
one:	.double 1.0
	ASM_SIZE_DIRECTIVE(one)
	ASM_TYPE_DIRECTIVE(limit,@object)
limit:	.double 0.29
	ASM_SIZE_DIRECTIVE(limit)
	ASM_TYPE_DIRECTIVE(ln2_2,@object)
ln2_2:	.tfloat 0.3465735902799726547086160
	ASM_SIZE_DIRECTIVE(ln2_2)

#ifdef PIC
#define MO(op) op##@GOTOFF(%edx)
#else
#define MO(op) op
#endif

	.text
ENTRY(__ieee754_atanh)
	movl	8(%esp), %ecx

	movl	%ecx, %eax
	andl	$0x7fffffff, %eax
	cmpl	$0x7ff00000, %eax
	jae	5f
7:

#ifdef PIC
	call	1f
1:	popl	%edx
	addl	$_GLOBAL_OFFSET_TABLE_+[.-1b], %edx
#endif

	andl	$0x80000000, %ecx // ECX == 0 iff X >= 0

	fldt	MO(ln2_2)	// 0.5*ln2
	xorl	%ecx, 8(%esp)
	fldl	4(%esp)		// |x| : 0.5*ln2
	fcoml	MO(half)	// |x| : 0.5*ln2
	fld	%st		// |x| : |x| : 0.5*ln2
	fnstsw			// |x| : |x| : 0.5*ln2
	sahf
	jae	2f
	fadd	%st, %st(1)	// |x| : 2*|x| : 0.5*ln2
	fld	%st		// |x| : |x| : 2*|x| : 0.5*ln2
	fsubrl	MO(one)		// 1-|x| : |x| : 2*|x| : 0.5*ln2
	fxch			// |x| : 1-|x| : 2*|x| : 0.5*ln2
	fmul	%st(2)		// 2*|x|^2 : 1-|x| : 2*|x| : 0.5*ln2
	fdivp			// (2*|x|^2)/(1-|x|) : 2*|x| : 0.5*ln2
	faddp			// 2*|x|+(2*|x|^2)/(1-|x|) : 0.5*ln2
	fcoml	MO(limit)	// 2*|x|+(2*|x|^2)/(1-|x|) : 0.5*ln2
	fnstsw			// 2*|x|+(2*|x|^2)/(1-|x|) : 0.5*ln2
	sahf
	jae	4f
	fyl2xp1			// 0.5*ln2*ld(1+2*|x|+(2*|x|^2)/(1-|x|))
	jecxz	3f
	fchs			// 0.5*ln2*ld(1+2*x+(2*x^2)/(1-x))
3:	ret

	.align ALIGNARG(4)
4:	faddl	MO(one)		// 1+2*|x|+(2*|x|^2)/(1-|x|) : 0.5*ln2
	fyl2x			// 0.5*ln2*ld(1+2*|x|+(2*|x|^2)/(1-|x|))
	jecxz	3f
	fchs			// 0.5*ln2*ld(1+2*x+(2*x^2)/(1-x))
3:	ret

	.align ALIGNARG(4)
2:	faddl	MO(one)		// 1+|x| : |x| : 0.5*ln2
	fxch			// |x| : 1+|x| : 0.5*ln2
	fsubrl	MO(one)		// 1-|x| : 1+|x| : 0.5*ln2
	fdivrp			// (1+|x|)/(1-|x|) : 0.5*ln2
	fyl2x			// 0.5*ln2*ld((1+|x|)/(1-|x|))
	jecxz	3f
	fchs			// 0.5*ln2*ld((1+x)/(1-x))
3:	ret

	// x == NaN or ±Inf
5:	ja	6f
	cmpl	$0, 4(%esp)
	je	7b
6:	fldl	4(%esp)
	ret
END(__ieee754_atanh)
