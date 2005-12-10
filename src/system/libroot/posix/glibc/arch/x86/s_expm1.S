/* ix87 specific implementation of exp(x)-1.
   Copyright (C) 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1996.
   Based on code by John C. Bowman <bowman@ipp-garching.mpg.de>.
   Corrections by H.J. Lu (hjl@gnu.ai.mit.edu), 1997.

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

	/* Using: e^x - 1 = 2^(x * log2(e)) - 1 */

#include <machine/asm.h>

#ifdef __ELF__
	.section .rodata
#else
	.text
#endif
	.align ALIGNARG(4)
	ASM_TYPE_DIRECTIVE(minus1,@object)
minus1:	.double -1.0
	ASM_SIZE_DIRECTIVE(minus1)
	ASM_TYPE_DIRECTIVE(one,@object)
one:	.double 1.0
	ASM_SIZE_DIRECTIVE(one)
	ASM_TYPE_DIRECTIVE(l2e,@object)
l2e:	.tfloat 1.442695040888963407359924681002
	ASM_SIZE_DIRECTIVE(l2e)

#ifdef PIC
#define MO(op) op##@GOTOFF(%edx)
#else
#define MO(op) op
#endif

	.text
ENTRY(__expm1)
	fldl	4(%esp)		// x
	fxam			// Is NaN or +-Inf?
	fstsw	%ax
	movb	$0x45, %ch
	andb	%ah, %ch
	cmpb	$0x40, %ch
	je	3f		// If +-0, jump.
#ifdef	PIC
	call	1f
1:	popl	%edx
	addl	$_GLOBAL_OFFSET_TABLE_+[.-1b], %edx
#endif
	cmpb	$0x05, %ch
	je	2f		// If +-Inf, jump.

	fldt	MO(l2e)		// log2(e) : x
	fmulp			// log2(e)*x
	fld	%st		// log2(e)*x : log2(e)*x
	frndint			// int(log2(e)*x) : log2(e)*x
	fsubr	%st, %st(1)	// int(log2(e)*x) : fract(log2(e)*x)
	fxch			// fract(log2(e)*x) : int(log2(e)*x)
	f2xm1			// 2^fract(log2(e)*x)-1 : int(log2(e)*x)
	fscale			// 2^(log2(e)*x)-2^int(log2(e)*x) : int(log2(e)*x)
	fxch			// int(log2(e)*x) : 2^(log2(e)*x)-2^int(log2(e)*x)
	fldl	MO(one)		// 1 : int(log2(e)*x) : 2^(log2(e)*x)-2^int(log2(e)*x)
	fscale			// 2^int(log2(e)*x) : int(log2(e)*x) : 2^(log2(e)*x)-2^int(log2(e)*x)
	fsubrl	MO(one)		// 1-2^int(log2(e)*x) : int(log2(e)*x) : 2^(log2(e)*x)-2^int(log2(e)*x)
	fstp	%st(1)		// 1-2^int(log2(e)*x) : 2^(log2(e)*x)-2^int(log2(e)*x)
	fsubrp	%st, %st(1)	// 2^(log2(e)*x)
	ret

2:	testl	$0x200, %eax	// Test sign.
	jz	3f		// If positive, jump.
	fstp	%st
	fldl	MO(minus1)	// Set result to -1.0.
3:	ret
END(__expm1)
weak_alias (__expm1, expm1)
