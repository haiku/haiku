/* Copyright (C) 1992, 1995, 1996 Free Software Foundation, Inc.
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

#include <stddef.h>
#include <search.h>

/* Insert ELEM into a doubly-linked list, after PREV.  */

void
insque (void *elem, void *prev)
{
  struct qelem *next = ((struct qelem *) prev)->q_forw;
  ((struct qelem *) prev)->q_forw = (struct qelem *) elem;
  if (next != NULL)
    next->q_back = (struct qelem *) elem;
  ((struct qelem *) elem)->q_forw = next;
  ((struct qelem *) elem)->q_back = (struct qelem *) prev;
}

/* Unlink ELEM from the doubly-linked list that it is in.  */

void
remque (void *elem)
{
  struct qelem *next = ((struct qelem *) elem)->q_forw;
  struct qelem *prev = ((struct qelem *) elem)->q_back;
  if (next != NULL)
    next->q_back = prev;
  if (prev != NULL)
    prev->q_forw = (struct qelem *) next;
}
