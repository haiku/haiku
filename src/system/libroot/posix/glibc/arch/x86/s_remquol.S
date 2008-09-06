/*
 * Written by Ulrich Drepper <drepper@cygnus.com>.
 * Based on e_remainder by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>
#include "bp-sym.h"
#include "bp-asm.h"

#define PARMS	LINKAGE		/* no space for saved regs */
#define DVDND	PARMS
#define DVSOR	DVDND+12
#define QUOP	DVSOR+12

	.text
ENTRY (BP_SYM (__remquol))
	ENTER

	fldt	DVSOR(%esp)
	fldt	DVDND(%esp)
1:	fprem1
	fstsw	%ax
	sahf
	jp	1b
	fstp	%st(1)
	/* Compute the congruent of the quotient.  */
	movl	%eax, %ecx
	shrl	$8, %eax
	shrl	$12, %ecx
	andl	$4, %ecx
	andl	$3, %eax
	orl	%eax, %ecx
	leal	(%ecx,%ecx,2),%ecx
	movl	$0xef2a60, %eax
	shrl	%cl, %eax
	andl	$7, %eax
	movl	QUOP(%esp), %ecx
	CHECK_BOUNDS_BOTH_WIDE (%ecx, QUOP(%esp), $4)
	movl	DVDND+8(%esp), %edx
	xorl	DVSOR+8(%esp), %edx
	testl	$0x8000, %edx
	jz	1f
	negl	%eax
1:	movl	%eax, (%ecx)

	LEAVE
	ret
END (BP_SYM (__remquol))
weak_alias (BP_SYM (__remquol), BP_SYM (remquol))
