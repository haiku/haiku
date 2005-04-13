/* An alternative to qsort, with an identical interface.
   This file is part of the GNU C Library.
   Copyright (C) 1992,95-97,99,2000,01,02 Free Software Foundation, Inc.
   Written by Mike Haertel, September 1988.

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

#include <alloca.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <memcopy.h>
#include <errno.h>

static void msort_with_tmp (void *b, size_t n, size_t s,
			    __compar_fn_t cmp, char *t);

static void
msort_with_tmp (void *b, size_t n, size_t s, __compar_fn_t cmp,
		char *t)
{
  char *tmp;
  char *b1, *b2;
  size_t n1, n2;

  if (n <= 1)
    return;

  n1 = n / 2;
  n2 = n - n1;
  b1 = b;
  b2 = (char *) b + (n1 * s);

  msort_with_tmp (b1, n1, s, cmp, t);
  msort_with_tmp (b2, n2, s, cmp, t);

  tmp = t;

  if (s == OPSIZ && (b1 - (char *) 0) % OPSIZ == 0)
    /* We are operating on aligned words.  Use direct word stores.  */
    while (n1 > 0 && n2 > 0)
      {
	if ((*cmp) (b1, b2) <= 0)
	  {
	    --n1;
	    *((op_t *) tmp)++ = *((op_t *) b1)++;
	  }
	else
	  {
	    --n2;
	    *((op_t *) tmp)++ = *((op_t *) b2)++;
	  }
      }
  else
    while (n1 > 0 && n2 > 0)
      {
	if ((*cmp) (b1, b2) <= 0)
	  {
	    tmp = (char *) __mempcpy (tmp, b1, s);
	    b1 += s;
	    --n1;
	  }
	else
	  {
	    tmp = (char *) __mempcpy (tmp, b2, s);
	    b2 += s;
	    --n2;
	  }
      }
  if (n1 > 0)
    memcpy (tmp, b1, n1 * s);
  memcpy (b, t, (n - n2) * s);
}

void
qsort (void *b, size_t n, size_t s, __compar_fn_t cmp)
{
  const size_t size = n * s;

  if (size < 1024)
    {
      void *buf = __alloca (size);

      /* The temporary array is small, so put it on the stack.  */
      msort_with_tmp (b, n, s, cmp, buf);
    }
  else
    {
      /* We should avoid allocating too much memory since this might
	 have to be backed up by swap space.  */
      static long int phys_pages;
      static int pagesize;

      if (phys_pages == 0)
	{
	  phys_pages = __sysconf (_SC_PHYS_PAGES);

	  if (phys_pages == -1)
	    /* Error while determining the memory size.  So let's
	       assume there is enough memory.  Otherwise the
	       implementer should provide a complete implementation of
	       the `sysconf' function.  */
	    phys_pages = (long int) (~0ul >> 1);

	  /* The following determines that we will never use more than
	     a quarter of the physical memory.  */
	  phys_pages /= 4;

	  pagesize = __sysconf (_SC_PAGESIZE);
	}

      /* Just a comment here.  We cannot compute
	   phys_pages * pagesize
	   and compare the needed amount of memory against this value.
	   The problem is that some systems might have more physical
	   memory then can be represented with a `size_t' value (when
	   measured in bytes.  */

      /* If the memory requirements are too high don't allocate memory.  */
      if (size / pagesize > (size_t) phys_pages)
	_quicksort (b, n, s, cmp);
      else
	{
	  /* It's somewhat large, so malloc it.  */
	  int save = errno;
	  char *tmp = malloc (size);
	  if (tmp == NULL)
	    {
	      /* Couldn't get space, so use the slower algorithm
		 that doesn't need a temporary array.  */
	      __set_errno (save);
	      _quicksort (b, n, s, cmp);
	    }
	  else
	    {
	      __set_errno (save);
	      msort_with_tmp (b, n, s, cmp, tmp);
	      free (tmp);
	    }
	}
    }
}
libc_hidden_def (qsort)
