/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Changes for long double by Ulrich Drepper <drepper@cygnus.com>
 * Changes for x86-64 by Andreas Jaeger <aj@suse.de>=09
 * Public domain.
 */

#include <machine/asm.h>

ENTRY(__floorl)
	fldt	8(%rsp)

	fstcw	-4(%rsp)		/* store fpu control word */

	/* We use here %edx although only the low 1 bits are defined.
	   But none of the operations should care and they are faster
	   than the 16 bit operations.  */
	movl	$0x400,%edx		/* round towards -oo */
	orl	-4(%rsp),%edx
	andl	$0xf7ff,%edx
	movl	%edx,-8(%rsp)
	fldcw	-8(%rsp)		/* load modified control word */

	frndint				/* round */

	fldcw	-4(%rsp)		/* restore original control word */

	ret
END (__floorl)
weak_alias (__floorl, floorl)
