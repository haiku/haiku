/* Compute maximum of two numbers, regarding NaN as missing argument.
   Copyright (C) 1997, 2002 Free Software Foundation, Inc.
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
ENTRY(__fmaxl)
	fldt	8(%rsp)		// x
	fldt	24(%rsp)	// x : y

	fucomi	%st(0), %st
	fcmovu	%st(1), %st	// now %st contains y if not NaN, x otherwise

	fxch

	fucomi	%st(1), %st
	fcmovb	%st(1), %st

	fstp	%st(1)

	ret
END(__fmaxl)
weak_alias (__fmaxl, fmaxl)
