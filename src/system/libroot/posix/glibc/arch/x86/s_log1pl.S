/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 *
 * Adapted for `long double' by Ulrich Drepper <drepper@cygnus.com>.
 */

#include <machine/asm.h>

RCSID("$NetBSD: s_log1p.S,v 1.7 1995/05/09 00:10:58 jtc Exp $")

#ifdef __ELF__
	.section .rodata
#else
	.text
#endif
	.align ALIGNARG(4)
	/* The fyl2xp1 can only be used for values in
		-1 + sqrt(2) / 2 <= x <= 1 - sqrt(2) / 2
	   0.29 is a safe value.
	*/
limit:	.tfloat 0.29
	/* Please note:	 we use a double value here.  Since 1.0 has
	   an exact representation this does not effect the accuracy
	   but it helps to optimize the code.  */
one:	.double 1.0

/*
 * Use the fyl2xp1 function when the argument is in the range -0.29 to 0.29,
 * otherwise fyl2x with the needed extra computation.
 */
	.text
ENTRY(__log1pl)
	fldln2

	fldt	4(%esp)

#ifdef PIC
	call	1f
1:	popl	%edx
	addl	$_GLOBAL_OFFSET_TABLE_+[.-1b], %edx
#endif

	fxam
	fnstsw
	fld	%st
	sahf
	jc	3f		// in case x is NaN or ±Inf
4:
	fabs
#ifdef PIC
	fldt	limit@GOTOFF(%edx)
#else
	fldt	limit
#endif
	fcompp
	fnstsw
	sahf
	jnc	2f

#ifdef PIC
	faddl	one@GOTOFF(%edx)
#else
	faddl	one
#endif
	fyl2x
	ret

2:	fyl2xp1
	ret

3:	jp	4b		// in case x is ±Inf
	fstp	%st(1)
	fstp	%st(1)
	ret

END (__log1pl)
weak_alias (__log1pl, log1pl)
