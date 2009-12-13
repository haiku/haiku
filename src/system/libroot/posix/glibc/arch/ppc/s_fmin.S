/* Floating-point minimum.  PowerPC version.
   Copyright (C) 1997, 1999 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

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

ENTRY(__fmin)
/* double [f1] fmin (double [f1] x, double [f2] y); */
	fcmpu	cr0,fp1,fp2
	bgt	cr0,0f		/* if x > y, neither x nor y can be NaN... */
	bnulr+	cr0
/* x and y are unordered, so one of x or y must be a NaN... */
	fcmpu	cr1,fp2,fp2
	bunlr	cr1
0:	fmr	fp1,fp2
	blr
END(__fmin)

weak_alias (__fmin,fmin)

/* It turns out that it's safe to use this code even for single-precision.  */
strong_alias(__fmin,__fminf)
weak_alias (__fmin,fminf)

#ifdef NO_LONG_DOUBLE
weak_alias (__fmin,__fminl)
weak_alias (__fmin,fminl)
#endif
