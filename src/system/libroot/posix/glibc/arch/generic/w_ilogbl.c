/* Copyright (C) 2012 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Adhemerval Zanella <azanella@linux.vnet.ibm.com>, 2011.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include <math.h>
#include <errno.h>
#include <limits.h>
#include <math_private.h>

/* wrapper ilogbl */
int
__ilogbl (long double x)
{
  int r = __ieee754_ilogbl (x);
  if (__builtin_expect (r == FP_ILOGB0, 0)
      || __builtin_expect (r == FP_ILOGBNAN, 0)
      || __builtin_expect (r == INT_MAX, 0))
    {
      __set_errno (EDOM);
      /*feraiseexcept (FE_INVALID);*/
    }
  return r;
}
weak_alias (__ilogbl, ilogbl)
