/* Copyright (C) 1999 Free Software Foundation, Inc.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include <math.h>
#include <stdlib.h>

/* There is no `long double' type, use the `double' implementations.  */
long double
__strtold_internal (const char *nptr, char **endptr, int group)
{
  return __strtod_internal (nptr, endptr, group);
}

long double
strtold (const char *nptr, char **endptr)
{
  return __strtod_internal (nptr, endptr, 0);
}
