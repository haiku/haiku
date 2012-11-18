/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Changes for long double by Ulrich Drepper <drepper@cygnus.com>.
 * Adopted for x86-64 by Andreas Jaeger <aj@suse.de>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$NetBSD: $")

ENTRY(__copysignl)
	movl	32(%rsp),%edx
	movl	16(%rsp),%eax
	andl	$0x8000,%edx
	andl	$0x7fff,%eax
	orl	%edx,%eax
	movl	%eax,16(%rsp)
	fldt	8(%rsp)
	ret
END (__copysignl)
weak_alias (__copysignl, copysignl)
