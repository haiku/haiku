/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 * Adapted for float type by Ulrich Drepper <drepper@cygnus.com>.
 *
 * Correct handling of y==-inf <drepper@gnu>
 */

#include <machine/asm.h>

RCSID("$NetBSD: $")

#ifdef __ELF__
	.section .rodata
#else
	.text
#endif

	.align ALIGNARG(4)
	ASM_TYPE_DIRECTIVE(zero_nan,@object)
zero_nan:
	.double 0.0
nan:	.byte 0, 0, 0, 0, 0, 0, 0xff, 0x7f
minus_zero:
	.byte 0, 0, 0, 0, 0, 0, 0, 0x80
	.byte 0, 0, 0, 0, 0, 0, 0xff, 0x7f
	ASM_SIZE_DIRECTIVE(zero_nan)


#ifdef PIC
#define MO(op) op##@GOTOFF(%ecx)
#define MOX(op,x,f) op##@GOTOFF(%ecx,x,f)
#else
#define MO(op) op
#define MOX(op,x,f) op(,x,f)
#endif


	.text
ENTRY(__ieee754_scalbf)
	flds	8(%esp)
	fxam
	fnstsw
	flds	4(%esp)
	andl	$0x4700, %eax
	cmpl	$0x0700, %eax
	je	1f
	andl	$0x4500, %eax
	cmpl	$0x0100, %eax
	je	2f
	fxam
	fnstsw
	andl	$0x4500, %eax
	cmpl	$0x0100, %eax
	je	3f
	fld	%st(1)
	frndint
	fcomp	%st(2)
	fnstsw
	sahf
	jne	4f
	fscale
	fstp	%st(1)
	ret

	/* y is -inf */
1:	fxam
#ifdef  PIC
        call    1f
1:      popl    %ecx
        addl    $_GLOBAL_OFFSET_TABLE_+[.-1b], %ecx
#endif
	fnstsw
	movl	4(%esp), %edx
	shrl	$5, %eax
	fstp	%st
	fstp	%st
	andl	$0x80000000, %edx
	andl	$8, %eax
	jnz	4f
	shrl	$27, %edx
	addl	%edx, %eax
	fldl	MOX(zero_nan, %eax, 1)
	ret

	/* The result is NaN, but we must not raise an exception.
	   So use a variable.  */
2:	fstp	%st
	fstp	%st
#ifdef  PIC
        call    1f
1:      popl    %ecx
        addl    $_GLOBAL_OFFSET_TABLE_+[.-1b], %ecx
#endif
	fldl	MO(nan)
	ret

	/* The first parameter is a NaN.  Return it.  */
3:	fstp	%st(1)
	ret

	/* Return NaN and raise the invalid exception.  */
4:	fstp	%st
	fstp	%st
	fldz
	fdiv	%st
	ret
END(__ieee754_scalbf)
