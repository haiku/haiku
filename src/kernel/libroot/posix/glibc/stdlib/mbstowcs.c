/* Copyright (C) 1991, 1992, 1996, 1997, 1999 Free Software Foundation, Inc.
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

#include <stdlib.h>
#include <string.h>
#include <wchar.h>


/* Convert the string of multibyte characters in S to `wchar_t's in
   PWCS, writing no more than N.  Return the number written,
   or (size_t) -1 if an invalid multibyte character is encountered.

   Attention: this function should NEVER be intentionally used.
   The interface is completely stupid.  The state is shared between
   all conversion functions.  You should use instead the restartable
   version `mbsrtowcs'.  */
size_t
mbstowcs (wchar_t *pwcs, const char *s, size_t n)
{
  mbstate_t state;

  memset (&state, '\0', sizeof state);
  /* Return how many we wrote (or maybe an error).  */
  return __mbsrtowcs (pwcs, &s, n, &state);
}
