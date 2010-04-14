/* Open a stream to a file.
   Copyright (C) 2007-2010 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2007.  */

#include <config.h>

/* Get the original definition of freopen.  It might be defined as a macro.  */
#define __need_FILE
#include <stdio.h>
#undef __need_FILE

static inline FILE *
orig_freopen (const char *filename, const char *mode, FILE *stream)
{
  return freopen (filename, mode, stream);
}

/* Specification.  */
#include <stdio.h>

#include <string.h>

FILE *
rpl_freopen (const char *filename, const char *mode, FILE *stream)
{
#if (defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__
  if (strcmp (filename, "/dev/null") == 0)
    filename = "NUL";
#endif

  return orig_freopen (filename, mode, stream);
}
