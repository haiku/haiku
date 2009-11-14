/* Provide a stub lchown function for systems that lack it.

   Copyright (C) 1998, 1999, 2002, 2004, 2006, 2007, 2009 Free
   Software Foundation, Inc.

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

/* If the system chown does not follow symlinks, we don't want it
   replaced by gnulib's chown, which does follow symlinks.  */
#if CHOWN_MODIFIES_SYMLINK
# define REPLACE_CHOWN 0
#endif
#include <unistd.h>

#include <errno.h>
#include <sys/stat.h>

/* Work just like chown, except when FILE is a symbolic link.
   In that case, set errno to EOPNOTSUPP and return -1.
   But if autoconf tests determined that chown modifies
   symlinks, then just call chown.  */

int
lchown (const char *file, uid_t uid, gid_t gid)
{
#if HAVE_CHOWN
# if ! CHOWN_MODIFIES_SYMLINK
  struct stat stats;

  if (lstat (file, &stats) == 0 && S_ISLNK (stats.st_mode))
    {
      errno = EOPNOTSUPP;
      return -1;
    }
# endif

  return chown (file, uid, gid);

#else /* !HAVE_CHOWN */
  errno = ENOSYS;
  return -1;
#endif
}
