/* Internals for openat-like functions.

   Copyright (C) 2005, 2006 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* written by Jim Meyering */

#include <errno.h>
#include <stdlib.h>

#define OPENAT_BUFFER_SIZE 512
char *openat_proc_name (char buf[OPENAT_BUFFER_SIZE], int fd, char const *file);

/* Some systems don't have ENOSYS.  */
#ifndef ENOSYS
# ifdef ENOTSUP
#  define ENOSYS ENOTSUP
# else
/* Some systems don't have ENOTSUP either.  */
#  define ENOSYS EINVAL
# endif
#endif

/* Some systems don't have EOPNOTSUPP.  */
#ifndef EOPNOTSUPP
# ifdef ENOTSUP
#  define EOPNOTSUPP ENOTSUP
# else
/* Some systems don't have ENOTSUP either.  */
#  define EOPNOTSUPP EINVAL
# endif
#endif

/* Trying to access a BUILD_PROC_NAME file will fail on systems without
   /proc support, and even on systems *with* ProcFS support.  Return
   nonzero if the failure may be legitimate, e.g., because /proc is not
   readable, or the particular .../fd/N directory is not present.  */
#define EXPECTED_ERRNO(Errno)			\
  ((Errno) == ENOTDIR || (Errno) == ENOENT	\
   || (Errno) == EPERM || (Errno) == EACCES	\
   || (Errno) == ENOSYS /* Solaris 8 */		\
   || (Errno) == EOPNOTSUPP /* FreeBSD */)
