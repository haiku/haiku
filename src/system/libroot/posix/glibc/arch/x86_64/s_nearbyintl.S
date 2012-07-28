/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */
/* Adapted for use as nearbyint by Ulrich Drepper <drepper@cygnus.com>.  */

#include <machine/asm.h>

ENTRY(__nearbyintl)
	fldt	8(%rsp)
	fnstcw	-4(%rsp)
	movl	-4(%rsp), %eax
	orl	$0x20, %eax
	movl	%eax, -8(%rsp)
	fldcw	-8(%rsp)
	frndint
	fclex
	fldcw	-4(%rsp)
	ret
END (__nearbyintl)
weak_alias (__nearbyintl, nearbyintl)
