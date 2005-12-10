/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$NetBSD: s_atanf.S,v 1.3 1995/05/08 23:51:33 jtc Exp $")

ENTRY(__atanf)
	flds	4(%esp)
	fld1
	fpatan
	ret
END (__atanf)
weak_alias (__atanf, atanf)
