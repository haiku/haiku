/* Work around an fstatat bug on Solaris 9.

   Copyright (C) 2006, 2009 Free Software Foundation, Inc.

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

/* Written by Paul Eggert and Jim Meyering.  */

#include <config.h>

#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>

#undef fstatat

/* fstatat should always follow symbolic links that end in /, but on
   Solaris 9 it doesn't if AT_SYMLINK_NOFOLLOW is specified.  This is
   the same problem that lstat.c addresses, so solve it in a similar
   way.  */

int
rpl_fstatat (int fd, char const *file, struct stat *st, int flag)
{
  int result = fstatat (fd, file, st, flag);

  if (result == 0 && (flag & AT_SYMLINK_NOFOLLOW) && S_ISLNK (st->st_mode)
      && file[strlen (file) - 1] == '/')
    {
      /* FILE refers to a symbolic link and the name ends with a slash.
	 Get info about the link's referent.  */
      result = fstatat (fd, file, st, flag & ~AT_SYMLINK_NOFOLLOW);
      if (result == 0 && ! S_ISDIR (st->st_mode))
	{
	  /* fstatat succeeded and FILE references a non-directory.
	     But it was specified via a name including a trailing
	     slash.  Fail with errno set to ENOTDIR to indicate the
	     contradiction.  */
	  errno = ENOTDIR;
	  return -1;
	}
    }

  return result;
}
