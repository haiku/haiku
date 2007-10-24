/* Set current rounding direction.
   Copyright (C) 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Andreas Schwab <schwab@issan.informatik.uni-dortmund.de>

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

#include <fenv.h>

int
fesetround (int round)
{
  fexcept_t fpcr;

  if (round & ~FE_UPWARD)
    /* ROUND is no valid rounding mode.  */
    return 1;

  __asm__ ("fmove%.l %!,%0" : "=dm" (fpcr));
  fpcr &= ~FE_UPWARD;
  fpcr |= round;
  __asm__ __volatile__ ("fmove%.l %0,%!" : : "dm" (fpcr));

  return 0;
}
