/* -*- buffer-read-only: t -*- vi: set ro: */
/* DO NOT EDIT! GENERATED AUTOMATICALLY! */
#line 1
/* Hook for making the close() function extensible.
   Copyright (C) 2009 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2009.

   This program is free software: you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>

/* Specification.  */
#include "close-hook.h"

#include <stdlib.h>
#include <unistd.h>

#undef close


/* Currently, this entire code is only needed for the handling of sockets
   on native Windows platforms.  */
#if WINDOWS_SOCKETS

/* The first and last link in the doubly linked list.
   Initially the list is empty.  */
static struct close_hook anchor = { &anchor, &anchor, NULL };

int
execute_close_hooks (int fd, const struct close_hook *remaining_list)
{
  if (remaining_list == &anchor)
    /* End of list reached.  */
    return close (fd);
  else
    return remaining_list->private_fn (fd, remaining_list->private_next);
}

int
execute_all_close_hooks (int fd)
{
  return execute_close_hooks (fd, anchor.private_next);
}

void
register_close_hook (close_hook_fn hook, struct close_hook *link)
{
  if (link->private_next == NULL && link->private_prev == NULL)
    {
      /* Add the link to the doubly linked list.  */
      link->private_next = anchor.private_next;
      link->private_prev = &anchor;
      link->private_fn = hook;
      anchor.private_next->private_prev = link;
      anchor.private_next = link;
    }
  else
    {
      /* The link is already in use.  */
      if (link->private_fn != hook)
	abort ();
    }
}

void
unregister_close_hook (struct close_hook *link)
{
  struct close_hook *next = link->private_next;
  struct close_hook *prev = link->private_prev;

  if (next != NULL && prev != NULL)
    {
      /* The link is in use.  Remove it from the doubly linked list.  */
      prev->private_next = next;
      next->private_prev = prev;
      /* Clear the link, to mark it unused.  */
      link->private_next = NULL;
      link->private_prev = NULL;
      link->private_fn = NULL;
    }
}

#endif
