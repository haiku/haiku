/* Copyright (C) 1991, 1997, 1999, 2000, 2002 Free Software Foundation, Inc.
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

#define	__NO_CTYPE
#include <ctype.h>

#define __ctype_tolower \
  ((uint32_t *) _NL_CURRENT (LC_CTYPE, _NL_CTYPE_TOLOWER) + 128)
#define __ctype_toupper \
  ((uint32_t *) _NL_CURRENT (LC_CTYPE, _NL_CTYPE_TOUPPER) + 128)

/* Real function versions of the non-ANSI ctype functions.  isblank is
   now in ISO C99 but we leave it here.  */

int
isblank (int c)
{
  return __isctype (c, _ISblank);
}

int
_tolower (int c)
{
  return __ctype_tolower[c];
}
int
_toupper (int c)
{
  return __ctype_toupper[c];
}

int
toascii (int c)
{
  return __toascii (c);
}
weak_alias (toascii, __toascii_l)

int
isascii (int c)
{
  return __isascii (c);
}
weak_alias (isascii, __isascii_l)


int
__isblank_l (int c, __locale_t l)
{
  return __isctype_l (c, _ISblank, l);
}
weak_alias (__isblank_l, isblank_l)
