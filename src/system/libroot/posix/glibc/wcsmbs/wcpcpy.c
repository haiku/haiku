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

#include <wchar.h>

#define __need_ptrdiff_t
#include <stddef.h>


/* Copy SRC to DEST, returning the address of the terminating L'\0' in
   DEST.  */
wchar_t *
__wcpcpy (dest, src)
     wchar_t *dest;
     const wchar_t *src;
{
  wchar_t *wcp = (wchar_t *) dest - 1;
  wint_t c;
  const ptrdiff_t off = src - dest + 1;

  do
    {
      c = wcp[off];
      *++wcp = c;
    }
  while (c != L'\0');

  return wcp;
}

weak_alias (__wcpcpy, wcpcpy)
