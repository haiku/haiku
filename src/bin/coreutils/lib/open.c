/* Open a descriptor to a file.
   Copyright (C) 2007-2008 Free Software Foundation, Inc.

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

/* Get the original definition of open.  It might be defined as a macro.  */
#define __need_system_fcntl_h
#include <fcntl.h>
#undef __need_system_fcntl_h
#include <sys/types.h>

static inline int
orig_open (const char *filename, int flags, mode_t mode)
{
  return open (filename, flags, mode);
}

/* Specification.  */
#include <fcntl.h>

#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

int
open (const char *filename, int flags, ...)
{
  mode_t mode;
  int fd;

  mode = 0;
  if (flags & O_CREAT)
    {
      va_list arg;
      va_start (arg, flags);

      /* If mode_t is narrower than int, use the promoted type (int),
	 not mode_t.  Use sizeof to guess whether mode_t is narrower;
	 we don't know of any practical counterexamples.  */
      mode = (sizeof (mode_t) < sizeof (int)
	      ? va_arg (arg, int)
	      : va_arg (arg, mode_t));

      va_end (arg);
    }

#if (defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__
  if (strcmp (filename, "/dev/null") == 0)
    filename = "NUL";
#endif

#if OPEN_TRAILING_SLASH_BUG
  /* If the filename ends in a slash and one of O_CREAT, O_WRONLY, O_RDWR
     is specified, then fail.
     Rationale: POSIX <http://www.opengroup.org/susv3/basedefs/xbd_chap04.html>
     says that
       "A pathname that contains at least one non-slash character and that
        ends with one or more trailing slashes shall be resolved as if a
        single dot character ( '.' ) were appended to the pathname."
     and
       "The special filename dot shall refer to the directory specified by
        its predecessor."
     If the named file already exists as a directory, then
       - if O_CREAT is specified, open() must fail because of the semantics
         of O_CREAT,
       - if O_WRONLY or O_RDWR is specified, open() must fail because POSIX
         <http://www.opengroup.org/susv3/functions/open.html> says that it
         fails with errno = EISDIR in this case.
     If the named file does not exist or does not name a directory, then
       - if O_CREAT is specified, open() must fail since open() cannot create
         directories,
       - if O_WRONLY or O_RDWR is specified, open() must fail because the
         file does not contain a '.' directory.  */
  if (flags & (O_CREAT | O_WRONLY | O_RDWR))
    {
      size_t len = strlen (filename);
      if (len > 0 && filename[len - 1] == '/')
	{
	  errno = EISDIR;
	  return -1;
	}
    }
#endif

  fd = orig_open (filename, flags, mode);

#if OPEN_TRAILING_SLASH_BUG
  /* If the filename ends in a slash and fd does not refer to a directory,
     then fail.
     Rationale: POSIX <http://www.opengroup.org/susv3/basedefs/xbd_chap04.html>
     says that
       "A pathname that contains at least one non-slash character and that
        ends with one or more trailing slashes shall be resolved as if a
        single dot character ( '.' ) were appended to the pathname."
     and
       "The special filename dot shall refer to the directory specified by
        its predecessor."
     If the named file without the slash is not a directory, open() must fail
     with ENOTDIR.  */
  if (fd >= 0)
    {
      size_t len = strlen (filename);
      if (len > 0 && filename[len - 1] == '/')
	{
	  struct stat statbuf;

	  if (fstat (fd, &statbuf) >= 0 && !S_ISDIR (statbuf.st_mode))
	    {
	      close (fd);
	      errno = ENOTDIR;
	      return -1;
	    }
	}
    }
#endif

#ifdef FCHDIR_REPLACEMENT
  if (fd >= 0)
    _gl_register_fd (fd, filename);
#endif

  return fd;
}
