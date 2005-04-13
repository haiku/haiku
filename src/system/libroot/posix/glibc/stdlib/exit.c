/* Copyright (C) 1991,95,96,97,99,2001,02 Free Software Foundation, Inc.
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
#include <stdlib.h>
#include <unistd.h>
#include "exit.h"

#ifdef HAVE_GNU_LD
#include "set-hooks.h"
DEFINE_HOOK (__libc_atexit, (void))
#endif


/* Call all functions registered with `atexit' and `on_exit',
   in the reverse of the order in which they were registered
   perform stdio cleanup, and terminate program execution with STATUS.  */
void
exit (int status)
{
  /* We do it this way to handle recursive calls to exit () made by
     the functions registered with `atexit' and `on_exit'. We call
     everyone on the list and use the status value in the last
     exit (). */
  while (__exit_funcs != NULL)
    {
      struct exit_function_list *old;

      while (__exit_funcs->idx > 0)
	{
	  const struct exit_function *const f =
	    &__exit_funcs->fns[--__exit_funcs->idx];
	  switch (f->flavor)
	    {
	    case ef_free:
	    case ef_us:
	      break;
	    case ef_on:
	      (*f->func.on.fn) (status, f->func.on.arg);
	      break;
	    case ef_at:
	      (*f->func.at) ();
	      break;
	    case ef_cxa:
	      (*f->func.cxa.fn) (f->func.cxa.arg, status);
	      break;
	    }
	}

      old = __exit_funcs;
      __exit_funcs = __exit_funcs->next;
      if (__exit_funcs != NULL)
	/* Don't free the last element in the chain, this is the statically
	   allocate element.  */
	free (old);
    }

#ifdef	HAVE_GNU_LD
  RUN_HOOK (__libc_atexit, ());
#else
  {
    extern void _cleanup (void);
    _cleanup ();
  }
#endif

  _exit (status);
}
libc_hidden_def (exit)
