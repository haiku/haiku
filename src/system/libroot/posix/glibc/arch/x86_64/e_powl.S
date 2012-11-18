/* ix87 specific implementation of pow function.
   Copyright (C) 1996, 1997, 1998, 1999, 2001, 2004, 2007
   Free Software Foundation, Inc.
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
	ASM_TYPE_DIRECTIVE(infinity,@object)
inf_zero:
infinity:
	.byte 0, 0, 0, 0, 0, 0, 0xf0, 0x7f
	ASM_SIZE_DIRECTIVE(infinity)
	ASM_TYPE_DIRECTIVE(zero,@object)
zero:	.double 0.0
	ASM_SIZE_DIRECTIVE(zero)
	ASM_TYPE_DIRECTIVE(minf_mzero,@object)
minf_mzero:
minfinity:
	.byte 0, 0, 0, 0, 0, 0, 0xf0, 0xff
mzero:
	.byte 0, 0, 0, 0, 0, 0, 0, 0x80
	ASM_SIZE_DIRECTIVE(minf_mzero)
	ASM_TYPE_DIRECTIVE(one,@object)
one:	.double 1.0
	ASM_SIZE_DIRECTIVE(one)
	ASM_TYPE_DIRECTIVE(limit,@object)
limit:	.double 0.29
	ASM_SIZE_DIRECTIVE(limit)
	ASM_TYPE_DIRECTIVE(p63,@object)
p63:
	.byte 0, 0, 0, 0, 0, 0, 0xe0, 0x43
	ASM_SIZE_DIRECTIVE(p63)

#ifdef PIC
#define MO(op) op##(%rip)
#else
#define MO(op) op
#endif

	.text
ENTRY(__ieee754_powl)
	fldt	24(%rsp)	// y
	fxam


	fnstsw
	movb	%ah, %dl
	andb	$0x45, %ah
	cmpb	$0x40, %ah	// is y == 0 ?
	je	11f

	cmpb	$0x05, %ah	// is y == ±inf ?
	je	12f

	cmpb	$0x01, %ah	// is y == NaN ?
	je	30f

	fldt	8(%rsp)		// x : y

	fxam
	fnstsw
	movb	%ah, %dh
	andb	$0x45, %ah
	cmpb	$0x40, %ah
	je	20f		// x is ±0

	cmpb	$0x05, %ah
	je	15f		// x is ±inf

	fxch			// y : x

	/* fistpll raises invalid exception for |y| >= 1L<<63.  */
	fldl	MO(p63)		// 1L<<63 : y : x
	fld	%st(1)		// y : 1L<<63 : y : x
	fabs			// |y| : 1L<<63 : y : x
	fcomip	%st(1), %st	// 1L<<63 : y : x
	fstp	%st(0)		// y : x
	jnc	2f

	/* First see whether `y' is a natural number.  In this case we
	   can use a more precise algorithm.  */
	fld	%st		// y : y : x
	fistpll	-8(%rsp)	// y : x
	fildll	-8(%rsp)	// int(y) : y : x
	fucomip	%st(1),%st	// y : x
	jne	2f

	/* OK, we have an integer value for y.  */
	mov	-8(%rsp),%eax
	mov	-4(%rsp),%edx
	orl	$0, %edx
	fstp	%st(0)		// x
	jns	4f		// y >= 0, jump
	fdivrl	MO(one)		// 1/x		(now referred to as x)
	negl	%eax
	adcl	$0, %edx
	negl	%edx
4:	fldl	MO(one)		// 1 : x
	fxch

6:	shrdl	$1, %edx, %eax
	jnc	5f
	fxch
	fmul	%st(1)		// x : ST*x
	fxch
5:	fmul	%st(0), %st	// x*x : ST*x
	shrl	$1, %edx
	movl	%eax, %ecx
	orl	%edx, %ecx
	jnz	6b
	fstp	%st(0)		// ST*x
	ret

	/* y is ±NAN */
30:	fldt	8(%rsp)		// x : y
	fldl	MO(one)		// 1.0 : x : y
	fucomip	%st(1),%st	// x : y
	je	31f
	fxch			// y : x
31:	fstp	%st(1)
	ret

	.align ALIGNARG(4)
2:	/* y is a real number.  */
	fxch			// x : y
	fldl	MO(one)		// 1.0 : x : y
	fldl	MO(limit)	// 0.29 : 1.0 : x : y
	fld	%st(2)		// x : 0.29 : 1.0 : x : y
	fsub	%st(2)		// x-1 : 0.29 : 1.0 : x : y
	fabs			// |x-1| : 0.29 : 1.0 : x : y
	fucompp			// 1.0 : x : y
	fnstsw
	fxch			// x : 1.0 : y
	test	$4500,%eax
	jz	7f
	fsub	%st(1)		// x-1 : 1.0 : y
	fyl2xp1			// log2(x) : y
	jmp	8f

7:	fyl2x			// log2(x) : y
8:	fmul	%st(1)		// y*log2(x) : y
	fxam
	fnstsw
	andb	$0x45, %ah
	cmpb	$0x05, %ah      // is y*log2(x) == ±inf ?
	je	28f
	fst	%st(1)		// y*log2(x) : y*log2(x)
	frndint			// int(y*log2(x)) : y*log2(x)
	fsubr	%st, %st(1)	// int(y*log2(x)) : fract(y*log2(x))
	fxch			// fract(y*log2(x)) : int(y*log2(x))
	f2xm1			// 2^fract(y*log2(x))-1 : int(y*log2(x))
	faddl	MO(one)		// 2^fract(y*log2(x)) : int(y*log2(x))
	fscale			// 2^fract(y*log2(x))*2^int(y*log2(x)) : int(y*log2(x))
	fstp	%st(1)		// 2^fract(y*log2(x))*2^int(y*log2(x))
	ret

28:	fstp	%st(1)		// y*log2(x)
	fldl	MO(one)		// 1 : y*log2(x)
	fscale			// 2^(y*log2(x)) : y*log2(x)
	fstp	%st(1)		// 2^(y*log2(x))
	ret

	// pow(x,±0) = 1
	.align ALIGNARG(4)
11:	fstp	%st(0)		// pop y
	fldl	MO(one)
	ret

	// y == ±inf
	.align ALIGNARG(4)
12:	fstp	%st(0)		// pop y
	fldl	MO(one)		// 1
	fldt	8(%rsp)		// x : 1
	fabs			// abs(x) : 1
	fucompp			// < 1, == 1, or > 1
	fnstsw
	andb	$0x45, %ah
	cmpb	$0x45, %ah
	je	13f		// jump if x is NaN

	cmpb	$0x40, %ah
	je	14f		// jump if |x| == 1

	shlb	$1, %ah
	xorb	%ah, %dl
	andl	$2, %edx
#ifdef PIC
	lea	inf_zero(%rip),%rcx
	fldl	(%rcx, %rdx, 4)
#else
	fldl	inf_zero(,%rdx, 4)
#endif
	ret

	.align ALIGNARG(4)
14:	fldl	MO(one)
	ret

	.align ALIGNARG(4)
13:	fldt	8(%rsp)		// load x == NaN
	ret

	.align ALIGNARG(4)
	// x is ±inf
15:	fstp	%st(0)		// y
	testb	$2, %dh
	jz	16f		// jump if x == +inf

	// We must find out whether y is an odd integer.
	fld	%st		// y : y
	fistpll	-8(%rsp)	// y
	fildll	-8(%rsp)	// int(y) : y
	fucomip %st(1),%st
	ffreep	%st		// <empty>
	jne	17f

	// OK, the value is an integer, but is it odd?
	mov	-8(%rsp), %eax
	mov	-4(%rsp), %edx
	andb	$1, %al
	jz	18f		// jump if not odd
	// It's an odd integer.
	shrl	$31, %edx
#ifdef PIC
	lea	minf_mzero(%rip),%rcx
	fldl	(%rcx, %rdx, 8)
#else
	fldl	minf_mzero(,%rdx, 8)
#endif
	ret

	.align ALIGNARG(4)
16:	fcompl	MO(zero)
	fnstsw
	shrl	$5, %eax
	andl	$8, %eax
#ifdef PIC
	lea	inf_zero(%rip),%rcx
	fldl	(%rcx, %rax, 1)
#else
	fldl	inf_zero(,%rax, 1)
#endif
	ret

	.align ALIGNARG(4)
17:	shll	$30, %edx	// sign bit for y in right position
18:	shrl	$31, %edx
#ifdef PIC
	lea	inf_zero(%rip),%rcx
	fldl	(%rcx, %rdx, 8)
#else
	fldl	inf_zero(,%rdx, 8)
#endif
	ret

	.align ALIGNARG(4)
	// x is ±0
20:	fstp	%st(0)		// y
	testb	$2, %dl
	jz	21f		// y > 0

	// x is ±0 and y is < 0.  We must find out whether y is an odd integer.
	testb	$2, %dh
	jz	25f

	fld	%st		// y : y
	fistpll	-8(%rsp)	// y
	fildll	-8(%rsp)	// int(y) : y
	fucomip	%st(1),%st
	ffreep	%st		// <empty>
	jne	26f

	// OK, the value is an integer, but is it odd?
	mov	-8(%rsp),%eax
	mov	-4(%rsp),%edx
	andb	$1, %al
	jz	27f		// jump if not odd
	// It's an odd integer.
	// Raise divide-by-zero exception and get minus infinity value.
	fldl	MO(one)
	fdivl	MO(zero)
	fchs
	ret

25:	fstp	%st(0)
26:
27:	// Raise divide-by-zero exception and get infinity value.
	fldl	MO(one)
	fdivl	MO(zero)
	ret

	.align ALIGNARG(4)
	// x is ±0 and y is > 0.  We must find out whether y is an odd integer.
21:	testb	$2, %dh
	jz	22f

	fld	%st		// y : y
	fistpll	-8(%rsp)	// y
	fildll	-8(%rsp)	// int(y) : y
	fucomip %st(1),%st
	ffreep	%st		// <empty>
	jne	23f

	// OK, the value is an integer, but is it odd?
	mov	-8(%rsp),%eax
	mov	-4(%rsp),%edx
	andb	$1, %al
	jz	24f		// jump if not odd
	// It's an odd integer.
	fldl	MO(mzero)
	ret

22:	fstp	%st(0)
23:
24:	fldl	MO(zero)
	ret

END(__ieee754_powl)
