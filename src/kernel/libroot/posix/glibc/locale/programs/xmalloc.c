/* xmalloc.c -- malloc with out of memory checking
   Copyright (C) 1990,91,92,93,94,95,96,97 Free Software Foundation, Inc.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if __STDC__
#define VOID void
#else
#define VOID char
#endif

#include <sys/types.h>

#if STDC_HEADERS || _LIBC
#include <stdlib.h>
static VOID *fixup_null_alloc __P ((size_t n));
VOID *xmalloc __P ((size_t n));
VOID *xcalloc __P ((size_t n, size_t s));
VOID *xrealloc __P ((VOID *p, size_t n));
#else
VOID *calloc ();
VOID *malloc ();
VOID *realloc ();
void free ();
#endif

#include <libintl.h>
#include "error.h"

#ifndef _
# define _(str) gettext (str)
#endif

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 4
#endif

/* Exit value when the requested amount of memory is not available.
   The caller may set it to some other value.  */
int xmalloc_exit_failure = EXIT_FAILURE;

static VOID *
fixup_null_alloc (n)
     size_t n;
{
  VOID *p;

  p = 0;
  if (n == 0)
    p = malloc ((size_t) 1);
  if (p == 0)
    error (xmalloc_exit_failure, 0, _("memory exhausted"));
  return p;
}

/* Allocate N bytes of memory dynamically, with error checking.  */

VOID *
xmalloc (n)
     size_t n;
{
  VOID *p;

  p = malloc (n);
  if (p == 0)
    p = fixup_null_alloc (n);
  return p;
}

/* Allocate memory for N elements of S bytes, with error checking.  */

VOID *
xcalloc (n, s)
     size_t n, s;
{
  VOID *p;

  p = calloc (n, s);
  if (p == 0)
    p = fixup_null_alloc (n);
  return p;
}

/* Change the size of an allocated block of memory P to N bytes,
   with error checking.
   If P is NULL, run xmalloc.  */

VOID *
xrealloc (p, n)
     VOID *p;
     size_t n;
{
  if (p == 0)
    return xmalloc (n);
  p = realloc (p, n);
  if (p == 0)
    p = fixup_null_alloc (n);
  return p;
}
