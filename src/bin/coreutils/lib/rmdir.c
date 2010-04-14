/* Work around rmdir bugs.

   Copyright (C) 1988, 1990, 1999, 2003-2006, 2009-2010 Free Software
   Foundation, Inc.

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

#include <config.h>

#include <unistd.h>

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#undef rmdir

/* Remove directory DIR.
   Return 0 if successful, -1 if not.  */

int
rpl_rmdir (char const *dir)
{
#if HAVE_RMDIR
  /* Work around cygwin 1.5.x bug where rmdir("dir/./") succeeds.  */
  size_t len = strlen (dir);
  int result;
  while (len && ISSLASH (dir[len - 1]))
    len--;
  if (len && dir[len - 1] == '.' && (1 == len || ISSLASH (dir[len - 2])))
    {
      errno = EINVAL;
      return -1;
    }
  result = rmdir (dir);
  /* Work around mingw bug, where rmdir("file/") fails with EINVAL
     instead of ENOTDIR.  We've already filtered out trailing ., the
     only reason allowed by POSIX for EINVAL.  */
  if (result == -1 && errno == EINVAL)
    errno = ENOTDIR;
  return result;

#else /* !HAVE_RMDIR */
  /* rmdir adapted from GNU tar.  FIXME: Delete this implementation in
     2010 if no one reports a system with missing rmdir.  */
  pid_t cpid;
  int status;
  struct stat statbuf;

  if (stat (dir, &statbuf) != 0)
    return -1;                  /* errno already set */

  if (!S_ISDIR (statbuf.st_mode))
    {
      errno = ENOTDIR;
      return -1;
    }

  cpid = fork ();
  switch (cpid)
    {
    case -1:                    /* cannot fork */
      return -1;                /* errno already set */

    case 0:                     /* child process */
      execl ("/bin/rmdir", "rmdir", dir, (char *) 0);
      _exit (1);

    default:                    /* parent process */

      /* Wait for kid to finish.  */

      while (wait (&status) != cpid)
        /* Do nothing.  */ ;

      if (status)
        {

          /* /bin/rmdir failed.  */

          errno = EIO;
          return -1;
        }
      return 0;
    }
#endif /* !HAVE_RMDIR */
}
