/* Copyright (C) 1995, 1996, 1997, 2001 Free Software Foundation, Inc.
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


/* Append SRC on the end of DEST.  */
wchar_t *
__wcscat (dest, src)
     wchar_t *dest;
     const wchar_t *src;
{
  register wchar_t *s1 = dest;
  register const wchar_t *s2 = src;
  wchar_t c;

  /* Find the end of the string.  */
  do
    c = *s1++;
  while (c != L'\0');

  /* Make S1 point before the next character, so we can increment
     it while memory is read (wins on pipelined cpus).	*/
  s1 -= 2;

  do
    {
      c = *s2++;
      *++s1 = c;
    }
  while (c != L'\0');

  return dest;
}
weak_alias (__wcscat, wcscat)
