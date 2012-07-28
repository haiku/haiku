/* Compute sine and cosine of argument.
   Copyright (C) 1997, 2000, 2001, 2005 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1997.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <machine/asm.h>
#include "bp-sym.h"
#include "bp-asm.h"

#define PARMS	LINKAGE		/* no space for saved regs */
#define ANGLE	PARMS
#define SINP	ANGLE+12
#define COSP	SINP+PTR_SIZE

	.text
ENTRY (BP_SYM (__sincosl))
	ENTER

	fldt	8(%rsp)
	fsincos
	fnstsw	%ax
	testl	$0x400,%eax
	jnz	1f
	fstpt	(%rsi)
	fstpt	(%rdi)

	LEAVE
	retq

1:	fldpi
	fadd	%st(0)
	fxch	%st(1)
2:	fprem1
	fnstsw	%ax
	testl	$0x400,%eax
	jnz	2b
	fstp	%st(1)
	fsincos
	fstpt	(%rsi)
	fstpt	(%rdi)

	LEAVE
	retq
END (BP_SYM (__sincosl))
weak_alias (BP_SYM (__sincosl), BP_SYM (sincosl))
