/* Copyright (C) 1998 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA.  */

/* Last ditch effort to support memmove: if user doesn't have
   memmove or bcopy, we offer this sluggish implementation. */

#include "config.h"
#ifndef HAVE_MEMMOVE

#include <sys/types.h>
#ifdef HAVE_MEMORY_H
# include <memory.h>
#endif

#ifndef VOID
# define VOID void
#endif

VOID *
memmove(dest, src, len)
  VOID *dest;
  const VOID *src;
  size_t len;
{
#ifdef HAVE_BCOPY
  bcopy(src, dest, len);

#else /*!HAVE_BCOPY*/
  char *dp = dest;
  const char *sp = src;

# ifdef HAVE_MEMCPY
  /* A special-case for non-overlapping regions, on the assumption
     that there is some hope that the sytem's memcpy() implementaion
     is better than our dumb fall-back one. */
  if ((dp < sp && dp+len < sp) || (sp < dp && sp+len < dp))
    return memcpy(dest, src, len);
# endif

  /* I tried real hard to avoid getting to this point.
     You *really* ought to upgrade your system's libraries;
	 the performance of this implementation sucks. */
  if (dp < sp)
    {
      while (len-- > 0)
	*dp++ = *sp++;
    }
  else
    {
      if (dp == sp)
	return dest;
      dp += len;
      sp += len;
      while (len-- > 0)
	*--dp = *--sp;
    }
#endif /*!HAVE_BCOPY*/

  return dest;
}

#endif	/*!HAVE_MEMMOVE*/
