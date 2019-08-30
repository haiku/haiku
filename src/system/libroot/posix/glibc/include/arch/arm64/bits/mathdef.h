/* Copyright (C) 1999, 2000, 2004, 2006, 2007
   Free Software Foundation, Inc.
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

#if !defined _MATH_H && !defined _COMPLEX_H
# error "Never use <bits/mathdef.h> directly; include <math.h> instead"
#endif

#if defined  __USE_ISOC99 && defined _MATH_H && !defined _MATH_H_MATHDEF
# define _MATH_H_MATHDEF	1

/* GCC does not promote `float' values to `double'.  */
typedef float float_t;		/* `float' expressions are evaluated as
				   `float'.  */
typedef double double_t;	/* `double' expressions are evaluated as
				   `double'.  */

#ifdef __FLT_EVAL_METHOD__
# if __FLT_EVAL_METHOD__ == -1
#  define FLT_EVAL_METHOD	2
# else
#  define FLT_EVAL_METHOD	__FLT_EVAL_METHOD__
# endif
#else
# define FLT_EVAL_METHOD	0
#endif

#endif	/* ISO C99 */
