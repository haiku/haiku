/* xmalloc.c -- safe versions of malloc and realloc */

/* Copyright (C) 1991-2009 Free Software Foundation, Inc.

   This file is part of GNU Bash, the GNU Bourne Again SHell.

   Bash is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Bash is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash.  If not, see <http://www.gnu.org/licenses/>.
*/

#if defined (HAVE_CONFIG_H)
#include <config.h>
#endif

#include "bashtypes.h"
#include <stdio.h>

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#if defined (HAVE_STDLIB_H)
#  include <stdlib.h>
#else
#  include "ansi_stdlib.h"
#endif /* HAVE_STDLIB_H */

#include "error.h"

#include "bashintl.h"

#if !defined (PTR_T)
#  if defined (__STDC__)
#    define PTR_T void *
#  else
#    define PTR_T char *
#  endif /* !__STDC__ */
#endif /* !PTR_T */

#if defined (HAVE_SBRK) && !HAVE_DECL_SBRK
extern char *sbrk();
#endif

static PTR_T lbreak;
static int brkfound;
static size_t allocated;

/* **************************************************************** */
/*								    */
/*		   Memory Allocation and Deallocation.		    */
/*								    */
/* **************************************************************** */

#if defined (HAVE_SBRK)
static size_t
findbrk ()
{
  if (brkfound == 0)
    {
      lbreak = (PTR_T)sbrk (0);
      brkfound++;
    }
  return (char *)sbrk (0) - (char *)lbreak;
}
#endif

/* Return a pointer to free()able block of memory large enough
   to hold BYTES number of bytes.  If the memory cannot be allocated,
   print an error message and abort. */
PTR_T
xmalloc (bytes)
     size_t bytes;
{
  PTR_T temp;

  temp = malloc (bytes);

  if (temp == 0)
    {
#if defined (HAVE_SBRK)
      allocated = findbrk ();
      fatal_error (_("xmalloc: cannot allocate %lu bytes (%lu bytes allocated)"), (unsigned long)bytes, (unsigned long)allocated);
#else
      fatal_error (_("xmalloc: cannot allocate %lu bytes"), (unsigned long)bytes);
#endif /* !HAVE_SBRK */
    }

  return (temp);
}

PTR_T
xrealloc (pointer, bytes)
     PTR_T pointer;
     size_t bytes;
{
  PTR_T temp;

  temp = pointer ? realloc (pointer, bytes) : malloc (bytes);

  if (temp == 0)
    {
#if defined (HAVE_SBRK)
      allocated = findbrk ();
      fatal_error (_("xrealloc: cannot reallocate %lu bytes (%lu bytes allocated)"), (unsigned long)bytes, (unsigned long)allocated);
#else
      fatal_error (_("xrealloc: cannot allocate %lu bytes"), (unsigned long)bytes);
#endif /* !HAVE_SBRK */
    }

  return (temp);
}

/* Use this as the function to call when adding unwind protects so we
   don't need to know what free() returns. */
void
xfree (string)
     PTR_T string;
{
  if (string)
    free (string);
}

#ifdef USING_BASH_MALLOC
#include <malloc/shmalloc.h>

PTR_T
sh_xmalloc (bytes, file, line)
     size_t bytes;
     char *file;
     int line;
{
  PTR_T temp;

  temp = sh_malloc (bytes, file, line);

  if (temp == 0)
    {
#if defined (HAVE_SBRK)
      allocated = findbrk ();
      fatal_error (_("xmalloc: %s:%d: cannot allocate %lu bytes (%lu bytes allocated)"), file, line, (unsigned long)bytes, (unsigned long)allocated);
#else
      fatal_error (_("xmalloc: %s:%d: cannot allocate %lu bytes"), file, line, (unsigned long)bytes);
#endif /* !HAVE_SBRK */
    }

  return (temp);
}

PTR_T
sh_xrealloc (pointer, bytes, file, line)
     PTR_T pointer;
     size_t bytes;
     char *file;
     int line;
{
  PTR_T temp;

  temp = pointer ? sh_realloc (pointer, bytes, file, line) : sh_malloc (bytes, file, line);

  if (temp == 0)
    {
#if defined (HAVE_SBRK)
      allocated = findbrk ();
      fatal_error (_("xrealloc: %s:%d: cannot reallocate %lu bytes (%lu bytes allocated)"), file, line, (unsigned long)bytes, (unsigned long)allocated);
#else
      fatal_error (_("xrealloc: %s:%d: cannot allocate %lu bytes"), file, line, (unsigned long)bytes);
#endif /* !HAVE_SBRK */
    }

  return (temp);
}

void
sh_xfree (string, file, line)
     PTR_T string;
     char *file;
     int line;
{
  if (string)
    sh_free (string, file, line);
}
#endif
