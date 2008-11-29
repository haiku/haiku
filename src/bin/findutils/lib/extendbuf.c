/* extendbuf.c -- manage a dynamically-allocated buffer

   Copyright 2004, 2005 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/* Written by James Yougnman <jay@gnu.org>. */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#include "xalloc.h"
#include "extendbuf.h"


/* We initially use a small default size to ensure that this code 
 * gets exercised. 
 */
#ifndef SIZE_DEFAULT
# define SIZE_DEFAULT 16
#endif

static size_t
decide_size(size_t current, size_t wanted)
{
  size_t newsize;
  
  if (0 == current)
    newsize = SIZE_DEFAULT;
  else
    newsize = current;
  
  while (newsize < wanted)
    {
      if (2 * newsize < newsize)
	xalloc_die ();
      newsize *= 2;
    }
  return newsize;
}


void *
extendbuf(void* existing, size_t wanted, size_t *allocated)
{
  int saved_errno;
  size_t newsize;
  void *result; /* leave uninitialised to allow static code checkers to identify bugs */

  saved_errno = errno;
  
  assert(wanted > 0u);
  newsize = decide_size(*allocated, wanted);
  
  if ( (*allocated) == 0 )
    {
      /* Sanity check: If there is no existing allocation size, there
       * must be no existing allocated buffer.
       */
      assert(NULL == existing);

      (*allocated) = newsize;
      result = xmalloc(newsize);
    }
  else
    {
      if (newsize != (*allocated) )
	{
	  (*allocated) = newsize;
	  result = xrealloc (existing, newsize);
	  
	}
      else
	{
	  result = existing;
	}
    }
  
  if (result)
    {
      /* xmalloc() or xrealloc() may have changed errno, but in the
	 success case we want to preserve the previous value.
      */
      errno = saved_errno;
    }
  return result;
}
