/* Internals for openat-like functions.

   Copyright (C) 2005, 2006, 2009 Free Software Foundation, Inc.

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

#include <errno.h>
#include <stdlib.h>

#define OPENAT_BUFFER_SIZE 512
char *openat_proc_name (char buf[OPENAT_BUFFER_SIZE], int fd, char const *file);

/* Trying to access a BUILD_PROC_NAME file will fail on systems without
   /proc support, and even on systems *with* ProcFS support.  Return
   nonzero if the failure may be legitimate, e.g., because /proc is not
   readable, or the particular .../fd/N directory is not present.  */
#define EXPECTED_ERRNO(Errno)			\
  ((Errno) == ENOTDIR || (Errno) == ENOENT	\
   || (Errno) == EPERM || (Errno) == EACCES	\
   || (Errno) == ENOSYS /* Solaris 8 */		\
   || (Errno) == EOPNOTSUPP /* FreeBSD */)
