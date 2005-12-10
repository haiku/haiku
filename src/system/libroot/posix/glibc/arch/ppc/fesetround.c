/* Set current rounding direction.
   Copyright (C) 1997 Free Software Foundation, Inc.
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

#include <fenv_libc.h>

int
fesetround (int round)
{
  fenv_union_t u;

  if ((unsigned int) round > 3)
    return 1;

  /* Get the current state.  */
  u.fenv = fegetenv_register ();

  /* Set the relevant bits.  */
  u.l[1] = (u.l[1] & ~3)  |  (round & 3);

  /* Put the new state in effect.  */
  fesetenv_register (u.fenv);

  return 0;
}
