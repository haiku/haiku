/* POSIX compatible write() function.
   Copyright (C) 2008-2010 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2008.

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

/* Specification.  */
#include <unistd.h>

/* Replace this function only if module 'sigpipe' is requested.  */
#if GNULIB_SIGPIPE

/* On native Windows platforms, SIGPIPE does not exist.  When write() is
   called on a pipe with no readers, WriteFile() fails with error
   GetLastError() = ERROR_NO_DATA, and write() in consequence fails with
   error EINVAL.  */

# if (defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__

#  include <errno.h>
#  include <signal.h>
#  include <io.h>

#  define WIN32_LEAN_AND_MEAN  /* avoid including junk */
#  include <windows.h>

ssize_t
rpl_write (int fd, const void *buf, size_t count)
#undef write
{
  ssize_t ret = write (fd, buf, count);

  if (ret < 0)
    {
      if (GetLastError () == ERROR_NO_DATA
          && GetFileType ((HANDLE) _get_osfhandle (fd)) == FILE_TYPE_PIPE)
        {
          /* Try to raise signal SIGPIPE.  */
          raise (SIGPIPE);
          /* If it is currently blocked or ignored, change errno from EINVAL
             to EPIPE.  */
          errno = EPIPE;
        }
    }
  return ret;
}

# endif
#endif
