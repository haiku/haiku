/* ix87 specific implementation of complex exponential function for double.
   Copyright (C) 1997 Free Software Foundation, Inc.
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

#include <sysdep.h>

#ifdef __ELF__
	.section .rodata
#else
	.text
#endif
	.align ALIGNARG(4)
	ASM_TYPE_DIRECTIVE(huge_nan_null_null,@object)
huge_nan_null_null:
	.byte 0, 0, 0x80, 0x7f
	.byte 0, 0, 0xc0, 0x7f
	.float	0.0
zero:	.float	0.0
infinity:
	.byte 0, 0, 0x80, 0x7f
	.byte 0, 0, 0xc0, 0x7f
	.float 0.0
	.byte 0, 0, 0, 0x80
	ASM_SIZE_DIRECTIVE(huge_nan_null_null)

	ASM_TYPE_DIRECTIVE(twopi,@object)
twopi:
	.byte 0x35, 0xc2, 0x68, 0x21, 0xa2, 0xda, 0xf, 0xc9, 0x1, 0x40
	.byte 0, 0, 0, 0, 0, 0
	ASM_SIZE_DIRECTIVE(twopi)

	ASM_TYPE_DIRECTIVE(l2e,@object)
l2e:
	.byte 0xbc, 0xf0, 0x17, 0x5c, 0x29, 0x3b, 0xaa, 0xb8, 0xff, 0x3f
	.byte 0, 0, 0, 0, 0, 0
	ASM_SIZE_DIRECTIVE(l2e)

	ASM_TYPE_DIRECTIVE(one,@object)
one:	.double 1.0
	ASM_SIZE_DIRECTIVE(one)


#ifdef PIC
#define MO(op) op##@GOTOFF(%ecx)
#define MOX(op,x,f) op##@GOTOFF(%ecx,x,f)
#else
#define MO(op) op
#define MOX(op,x,f) op(,x,f)
#endif

	.text
ENTRY(__cexpf)
	flds	4(%esp)			/* x */
	fxam
	fnstsw
	flds	8(%esp)			/* y : x */
#ifdef  PIC
        call    1f
1:      popl    %ecx
        addl    $_GLOBAL_OFFSET_TABLE_+[.-1b], %ecx
#endif
	movb	%ah, %dh
	andb	$0x45, %ah
	cmpb	$0x05, %ah
	je	1f			/* Jump if real part is +-Inf */
	cmpb	$0x01, %ah
	je	2f			/* Jump if real part is NaN */

	fxam				/* y : x */
	fnstsw
	/* If the imaginary part is not finite we return NaN+i NaN, as
	   for the case when the real part is NaN.  A test for +-Inf and
	   NaN would be necessary.  But since we know the stack register
	   we applied `fxam' to is not empty we can simply use one test.
	   Check your FPU manual for more information.  */
	andb	$0x01, %ah
	cmpb	$0x01, %ah
	je	20f

	/* We have finite numbers in the real and imaginary part.  Do
	   the real work now.  */
	fxch			/* x : y */
	fldt	MO(l2e)		/* log2(e) : x : y */
	fmulp			/* x * log2(e) : y */
	fld	%st		/* x * log2(e) : x * log2(e) : y */
	frndint			/* int(x * log2(e)) : x * log2(e) : y */
	fsubr	%st, %st(1)	/* int(x * log2(e)) : frac(x * log2(e)) : y */
	fxch			/* frac(x * log2(e)) : int(x * log2(e)) : y */
	f2xm1			/* 2^frac(x * log2(e))-1 : int(x * log2(e)) : y */
	faddl	MO(one)		/* 2^frac(x * log2(e)) : int(x * log2(e)) : y */
	fscale			/* e^x : int(x * log2(e)) : y */
	fst	%st(1)		/* e^x : e^x : y */
	fxch	%st(2)		/* y : e^x : e^x */
	fsincos			/* cos(y) : sin(y) : e^x : e^x */
	fnstsw
	testl	$0x400, %eax
	jnz	7f
	fmulp	%st, %st(3)	/* sin(y) : e^x : e^x * cos(y) */
	fmulp	%st, %st(1)	/* e^x * sin(y) : e^x * cos(y) */
	subl	$8, %esp
	fstps	4(%esp)
	fstps	(%esp)
	popl	%eax
	popl	%edx
	ret

	/* We have to reduce the argument to fsincos.  */
	.align ALIGNARG(4)
7:	fldt	MO(twopi)	/* 2*pi : y : e^x : e^x */
	fxch			/* y : 2*pi : e^x : e^x */
8:	fprem1			/* y%(2*pi) : 2*pi : e^x : e^x */
	fnstsw
	testl	$0x400, %eax
	jnz	8b
	fstp	%st(1)		/* y%(2*pi) : e^x : e^x */
	fsincos			/* cos(y) : sin(y) : e^x : e^x */
	fmulp	%st, %st(3)
	fmulp	%st, %st(1)
	subl	$8, %esp
	fstps	4(%esp)
	fstps	(%esp)
	popl	%eax
	popl	%edx
	ret

	/* The real part is +-inf.  We must make further differences.  */
	.align ALIGNARG(4)
1:	fxam			/* y : x */
	fnstsw
	movb	%ah, %dl
	testb	$0x01, %ah	/* See above why 0x01 is usable here.  */
	jne	3f


	/* The real part is +-Inf and the imaginary part is finite.  */
	andl	$0x245, %edx
	cmpb	$0x40, %dl	/* Imaginary part == 0?  */
	je	4f		/* Yes ->  */

	fxch			/* x : y */
	shrl	$6, %edx
	fstp	%st(0)		/* y */ /* Drop the real part.  */
	andl	$8, %edx	/* This puts the sign bit of the real part
				   in bit 3.  So we can use it to index a
				   small array to select 0 or Inf.  */
	fsincos			/* cos(y) : sin(y) */
	fnstsw
	testl	$0x0400, %eax
	jnz	5f
	fxch
	ftst
	fnstsw
	fstp	%st(0)
	shll	$23, %eax
	andl	$0x80000000, %eax
	orl	MOX(huge_nan_null_null,%edx,1), %eax
	movl	MOX(huge_nan_null_null,%edx,1), %ecx
	movl	%eax, %edx
	ftst
	fnstsw
	fstp	%st(0)
	shll	$23, %eax
	andl	$0x80000000, %eax
	orl	%ecx, %eax
	ret
	/* We must reduce the argument to fsincos.  */
	.align ALIGNARG(4)
5:	fldt	MO(twopi)
	fxch
6:	fprem1
	fnstsw
	testl	$0x400, %eax
	jnz	6b
	fstp	%st(1)
	fsincos
	fxch
	ftst
	fnstsw
	fstp	%st(0)
	shll	$23, %eax
	andl	$0x80000000, %eax
	orl	MOX(huge_nan_null_null,%edx,1), %eax
	movl	MOX(huge_nan_null_null,%edx,1), %ecx
	movl	%eax, %edx
	ftst
	fnstsw
	fstp	%st(0)
	shll	$23, %eax
	andl	$0x80000000, %eax
	orl	%ecx, %eax
	ret

	/* The real part is +-Inf and the imaginary part is +-0.  So return
	   +-Inf+-0i.  */
	.align ALIGNARG(4)
4:	subl	$4, %esp
	fstps	(%esp)
	shrl	$6, %edx
	fstp	%st(0)
	andl	$8, %edx
	movl	MOX(huge_nan_null_null,%edx,1), %eax
	popl	%edx
	ret

	/* The real part is +-Inf, the imaginary is also is not finite.  */
	.align ALIGNARG(4)
3:	fstp	%st(0)
	fstp	%st(0)		/* <empty> */
	andb	$0x45, %ah
	andb	$0x47, %dh
	xorb	%dh, %ah
	jnz	30f
	flds	MO(infinity)	/* Raise invalid exception.  */
	fmuls	MO(zero)
	fstp	%st(0)
30:	movl	%edx, %eax
	shrl	$6, %edx
	shll	$3, %eax
	andl	$8, %edx
	andl	$16, %eax
	orl	%eax, %edx

	movl	MOX(huge_nan_null_null,%edx,1), %eax
	movl	MOX(huge_nan_null_null+4,%edx,1), %edx
	ret

	/* The real part is NaN.  */
	.align ALIGNARG(4)
20:	flds	MO(infinity)		/* Raise invalid exception.  */
	fmuls	MO(zero)
	fstp	%st(0)
2:	fstp	%st(0)
	fstp	%st(0)
	movl	MO(huge_nan_null_null+4), %eax
	movl	%eax, %edx
	ret

END(__cexpf)
weak_alias (__cexpf, cexpf)
