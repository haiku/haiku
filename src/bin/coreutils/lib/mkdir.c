/* On some systems, mkdir ("foo/", 0700) fails because of the trailing
   slash.  On those systems, this wrapper removes the trailing slash.

   Copyright (C) 2001, 2003, 2006, 2008 Free Software Foundation, Inc.

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

/* written by Jim Meyering */

#include <config.h>

/* Specification.  */
#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dirname.h"
#include "xalloc.h"

/* Disable the definition of mkdir to rpl_mkdir (from the <sys/stat.h>
   substitute) in this file.  Otherwise, we'd get an endless recursion.  */
#undef mkdir

/* mingw's _mkdir() function has 1 argument, but we pass 2 arguments.
   Additionally, it declares _mkdir (and depending on compile flags, an
   alias mkdir), only in the nonstandard io.h.  */
#if (defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__
# define mkdir(name,mode) _mkdir (name)
# define maybe_unused _UNUSED_PARAMETER_
#else
# define maybe_unused /* empty */
#endif

/* This function is required at least for NetBSD 1.5.2.  */

int
rpl_mkdir (char const *dir, mode_t mode maybe_unused)
{
  int ret_val;
  char *tmp_dir;
  size_t len = strlen (dir);

  if (len && dir[len - 1] == '/')
    {
      tmp_dir = xstrdup (dir);
      strip_trailing_slashes (tmp_dir);
    }
  else
    {
      tmp_dir = (char *) dir;
    }

  ret_val = mkdir (tmp_dir, mode);

  if (tmp_dir != dir)
    free (tmp_dir);

  return ret_val;
}
