/* provide a replacement fdopendir function
   Copyright (C) 2004-2009 Free Software Foundation, Inc.

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

#include <dirent.h>

#include <stdlib.h>
#include <unistd.h>

#include "openat.h"
#include "openat-priv.h"
#include "save-cwd.h"

#if GNULIB_DIRENT_SAFER
# include "dirent--.h"
#endif

/* Replacement for Solaris' function by the same name.
   <http://www.google.com/search?q=fdopendir+site:docs.sun.com>
   First, try to simulate it via opendir ("/proc/self/fd/FD").  Failing
   that, simulate it by using fchdir metadata, or by doing
   save_cwd/fchdir/opendir(".")/restore_cwd.
   If either the save_cwd or the restore_cwd fails (relatively unlikely),
   then give a diagnostic and exit nonzero.
   Otherwise, this function works just like Solaris' fdopendir.

   W A R N I N G:
   Unlike other fd-related functions, this one effectively consumes
   its FD parameter.  The caller should not close or otherwise
   manipulate FD if this function returns successfully.  Also, this
   implementation does not guarantee that dirfd(fdopendir(n))==n;
   the open directory stream may use a clone of FD, or have no
   associated fd at all.  */
DIR *
fdopendir (int fd)
{
  int saved_errno;
  DIR *dir;

  char buf[OPENAT_BUFFER_SIZE];
  char *proc_file = openat_proc_name (buf, fd, ".");
  if (proc_file)
    {
      dir = opendir (proc_file);
      saved_errno = errno;
    }
  else
    {
      dir = NULL;
      saved_errno = EOPNOTSUPP;
    }

  /* If the syscall fails with an expected errno value, resort to
     save_cwd/restore_cwd.  */
  if (! dir && EXPECTED_ERRNO (saved_errno))
    {
#if REPLACE_FCHDIR
      const char *name = _gl_directory_name (fd);
      if (name)
        dir = opendir (name);
      saved_errno = errno;
#else /* !REPLACE_FCHDIR */
      struct saved_cwd saved_cwd;
      if (save_cwd (&saved_cwd) != 0)
	openat_save_fail (errno);

      if (fchdir (fd) != 0)
	{
	  dir = NULL;
	  saved_errno = errno;
	}
      else
	{
	  dir = opendir (".");
	  saved_errno = errno;

	  if (restore_cwd (&saved_cwd) != 0)
	    openat_restore_fail (errno);
	}

      free_cwd (&saved_cwd);
#endif /* !REPLACE_FCHDIR */
    }

  if (dir)
    close (fd);
  if (proc_file != buf)
    free (proc_file);
  errno = saved_errno;
  return dir;
}
