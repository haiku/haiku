/* Copyright (C) 1991,1993,1996-1999,2000 Free Software Foundation, Inc.
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

#include <stdio.h>

/* Generate a unique filename in P_tmpdir.  If S is NULL return NULL.
   This makes this function thread safe.  */
char *
tmpnam_r (char *s)
{
  if (s == NULL)
    return NULL;

  if (__path_search (s, L_tmpnam, NULL, NULL, 0))
    return NULL;
  if (__gen_tempname (s, __GT_NOCREATE))
    return NULL;

  return s;
}

link_warning (tmpnam_r,
	      "the use of `tmpnam_r' is dangerous, better use `mkstemp'")
