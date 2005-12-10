/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$NetBSD: s_log1pf.S,v 1.4 1995/05/09 00:13:05 jtc Exp $")

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
limit:	.float 0.29
one:	.float 1.0

/*
 * Use the fyl2xp1 function when the argument is in the range -0.29 to 0.29,
 * otherwise fyl2x with the needed extra computation.
 */
	.text
ENTRY(__log1pf)
	fldln2

	flds	4(%esp)

#ifdef	PIC
	call	1f
1:	popl	%edx
	addl	$_GLOBAL_OFFSET_TABLE_+[.-1b], %edx
#endif

	fxam
	fnstsw
	fld	%st
	sahf
	jc	3f		// in case x is NaN or ±Inf
4:	fabs
#ifdef PIC
	fcomps	limit@GOTOFF(%edx)
#else
	fcomps	limit
#endif
	fnstsw
	sahf
	jc	2f

#ifdef PIC
	fadds	one@GOTOFF(%edx)
#else
	fadds	one
#endif
	fyl2x
	ret

2:	fyl2xp1
	ret

3:	jp	4b		// in case x is ±Inf
	fstp	%st(1)
	fstp	%st(1)
	ret

END (__log1pf)
weak_alias (__log1pf, log1pf)
