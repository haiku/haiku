/* Copyright (C) 1999, 2001, 2002 Free Software Foundation, Inc.
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

#include <assert.h>
#include <stdlib.h>
#include <atomicity.h>
#include "exit.h"
#include <fork.h>

/* If D is non-NULL, call all functions registered with `__cxa_atexit'
   with the same dso handle.  Otherwise, if D is NULL, do nothing.  */
void
__cxa_finalize (void *d)
{
  struct exit_function_list *funcs;

  for (funcs = __exit_funcs; funcs; funcs = funcs->next)
    {
      struct exit_function *f;

      for (f = &funcs->fns[funcs->idx - 1]; f >= &funcs->fns[0]; --f)
	if ((d == NULL || d == f->func.cxa.dso_handle)
	    /* We don't want to run this cleanup more than once.  */
	    && compare_and_swap (&f->flavor, ef_cxa, ef_free))
	  (*f->func.cxa.fn) (f->func.cxa.arg, 0);
    }

  /* Remove the registered fork handlers.  */
#ifdef UNREGISTER_ATFORK
  UNREGISTER_ATFORK (d);
#endif
}
