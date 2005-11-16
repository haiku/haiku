/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$NetBSD: s_scalbn.S,v 1.4 1995/05/09 00:19:06 jtc Exp $")

ENTRY(__scalbn)
	fildl	12(%esp)
	fldl	4(%esp)
	fscale
	fstp	%st(1)
	ret
END (__scalbn)
weak_alias (__scalbn, scalbn)
strong_alias (__scalbn, __scalbln)
weak_alias (__scalbn, scalbln)
