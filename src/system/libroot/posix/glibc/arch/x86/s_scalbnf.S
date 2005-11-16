/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$NetBSD: s_scalbnf.S,v 1.3 1995/05/09 00:19:59 jtc Exp $")

ENTRY(__scalbnf)
	fildl	8(%esp)
	flds	4(%esp)
	fscale
	fstp	%st(1)
	ret
END (__scalbnf)
weak_alias (__scalbnf, scalbnf)
strong_alias (__scalbnf, __scalblnf)
weak_alias (__scalbnf, scalblnf)
