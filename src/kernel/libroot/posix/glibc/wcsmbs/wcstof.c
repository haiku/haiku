/* Copyright (C) 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1996.

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


/* The actual implementation for all floating point sizes is in strtod.c.
   These macros tell it to produce the `float' version, `wcstof'.  */

#define	FLOAT		float
#define	FLT		FLT
#ifdef USE_IN_EXTENDED_LOCALE_MODEL
# define STRTOF		__wcstof_l
#else
# define STRTOF		wcstof
#endif
#define	MPN2FLOAT	__mpn_construct_float
#define	FLOAT_HUGE_VAL	HUGE_VALF
#define	USE_WIDE_CHAR	1
#define SET_MANTISSA(flt, mant) \
  do { union ieee754_float u;						      \
       u.f = (flt);							      \
       if ((mant & 0x7fffff) == 0)					      \
	 mant = 0x400000;						      \
       u.ieee.mantissa = (mant) & 0x7fffff;				      \
       (flt) = u.f;							      \
  } while (0)

#include <stdlib/strtod.c>
