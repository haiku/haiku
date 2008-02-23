/* realloc() function that is glibc compatible.

   Copyright (C) 1997, 2003, 2004, 2006, 2007 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* written by Jim Meyering and Bruno Haible */

#include <config.h>
/* Only the AC_FUNC_REALLOC macro defines 'realloc' already in config.h.  */
#ifdef realloc
# define NEED_REALLOC_GNU
# undef realloc
#endif

/* Specification.  */
#include <stdlib.h>

#include <errno.h>

/* Call the system's malloc and realloc below.  */
#undef malloc
#undef realloc

/* Change the size of an allocated block of memory P to N bytes,
   with error checking.  If N is zero, change it to 1.  If P is NULL,
   use malloc.  */

void *
rpl_realloc (void *p, size_t n)
{
  void *result;

#ifdef NEED_REALLOC_GNU
  if (n == 0)
    {
      n = 1;

      /* In theory realloc might fail, so don't rely on it to free.  */
      free (p);
      p = NULL;
    }
#endif

  result = (p == NULL ? malloc (n) : realloc (p, n));

#if !HAVE_REALLOC_POSIX
  if (result == NULL)
    errno = ENOMEM;
#endif

  return result;
}
