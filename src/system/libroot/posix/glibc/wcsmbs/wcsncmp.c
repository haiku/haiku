/* Copyright (C) 1995, 1996 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1995.

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

#include <wchar.h>


/* Compare no more than N characters of S1 and S2,
   returning less than, equal to or greater than zero
   if S1 is lexicographically less than, equal to or
   greater than S2.  */
int
wcsncmp (s1, s2, n)
     const wchar_t *s1;
     const wchar_t *s2;
     size_t n;
{
  wint_t c1 = L'\0';
  wint_t c2 = L'\0';

  if (n >= 4)
    {
      size_t n4 = n >> 2;
      do
	{
	  c1 = (wint_t) *s1++;
	  c2 = (wint_t) *s2++;
	  if (c1 == L'\0' || c1 != c2)
	    return c1 - c2;
	  c1 = (wint_t) *s1++;
	  c2 = (wint_t) *s2++;
	  if (c1 == L'\0' || c1 != c2)
	    return c1 - c2;
	  c1 = (wint_t) *s1++;
	  c2 = (wint_t) *s2++;
	  if (c1 == L'\0' || c1 != c2)
	    return c1 - c2;
	  c1 = (wint_t) *s1++;
	  c2 = (wint_t) *s2++;
	  if (c1 == L'\0' || c1 != c2)
	    return c1 - c2;
	} while (--n4 > 0);
      n &= 3;
    }

  while (n > 0)
    {
      c1 = (wint_t) *s1++;
      c2 = (wint_t) *s2++;
      if (c1 == L'\0' || c1 != c2)
	return c1 - c2;
      n--;
    }

  return c1 - c2;
}
