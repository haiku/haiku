/* Open a stdio stream on an anonymous temporary file.  Generic/POSIX version.
   Copyright (C) 1991,93,1996-2000,2002,2003 Free Software Foundation, Inc.
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

#include <stdio_private.h>
#include <unistd.h>

#ifdef USE_IN_LIBIO
# include <iolibio.h>
# define __fdopen INTUSE(_IO_fdopen)
# ifndef tmpfile
#  define tmpfile __new_tmpfile
# endif
#endif

#ifndef GEN_THIS
# define GEN_THIS __GT_FILE
#endif

/* This returns a new stream opened on a temporary file (generated
   by tmpnam).  The file is opened with mode "w+b" (binary read/write).
   If we couldn't generate a unique filename or the file couldn't
   be opened, NULL is returned.  */
FILE *
tmpfile (void)
{
  char buf[FILENAME_MAX];
  int fd;
  FILE *f;

  if (__path_search (buf, FILENAME_MAX, NULL, "tmpf", 0))
    return NULL;
  fd = __gen_tempname (buf, GEN_THIS);
  if (fd < 0)
    return NULL;

  /* Note that this relies on the Unix semantics that
     a file is not really removed until it is closed.  */
  (void) __unlink (buf);

  if ((f = __fdopen (fd, "w+b")) == NULL)
    __close (fd);

  return f;
}

#if defined USE_IN_LIBIO && GEN_THIS == __GT_FILE /* Not for tmpfile64.  */
# undef tmpfile
# include <shlib-compat.h>
versioned_symbol (libc, __new_tmpfile, tmpfile, GLIBC_2_1);
#endif
