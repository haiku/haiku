/* Compute positive difference.
   Copyright (C) 1997, 1999, 2004 Free Software Foundation, Inc.
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

#include <sysdep.h>

	.text
ENTRY(__fdim)
	fldl	4(%esp)		// x
	fldl	12(%esp)	// x : y

	fucom	%st(1)
	fnstsw
	sahf
	jp	1f

	jc	3f

	fstp	%st(1)
	fldz
	jmp	2f

3:	fsubrp	%st, %st(1)
	ret

1:	fxam
	fnstsw
	andb	$0x45, %ah
	cmpb	$0x01, %ah
	je	2f

	fxch
2:	fstp	%st(1)
	ret
END(__fdim)
weak_alias (__fdim, fdim)
