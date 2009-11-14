/* Work around rename bugs in some systems.  On SunOS 4.1.1_U1
   and mips-dec-ultrix4.4, rename fails when the source file has
   a trailing slash.  On mingw, rename fails when the destination
   exists.

   Copyright (C) 2001, 2002, 2003, 2005, 2006, 2009 Free Software
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

/* written by Volker Borchert */

#include <config.h>

#include <stdio.h>

#undef rename

#if (defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__
/* The mingw rename has problems with trailing slashes; it also
   requires use of native Windows calls to allow atomic renames over
   existing files.  */

# include <errno.h>

# define WIN32_LEAN_AND_MEAN
# include <windows.h>

/* Rename the file SRC to DST.  This replacement is necessary on
   Windows, on which the system rename function will not replace
   an existing DST.  */
int
rpl_rename (char const *src, char const *dst)
{
  int error;

  /* MoveFileEx works if SRC is a directory without any flags,
     but fails with MOVEFILE_REPLACE_EXISTING, so try without
     flags first. */
  if (MoveFileEx (src, dst, 0))
    return 0;

  /* Retry with MOVEFILE_REPLACE_EXISTING if the move failed
   * due to the destination already existing. */
  error = GetLastError ();
  if (error == ERROR_FILE_EXISTS || error == ERROR_ALREADY_EXISTS)
    {
      if (MoveFileEx (src, dst, MOVEFILE_REPLACE_EXISTING))
        return 0;

      error = GetLastError ();
    }

  switch (error)
    {
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
    case ERROR_BAD_PATHNAME:
    case ERROR_DIRECTORY:
      errno = ENOENT;
      break;

    case ERROR_ACCESS_DENIED:
    case ERROR_SHARING_VIOLATION:
      errno = EACCES;
      break;

    case ERROR_OUTOFMEMORY:
      errno = ENOMEM;
      break;

    case ERROR_CURRENT_DIRECTORY:
      errno = EBUSY;
      break;

    case ERROR_NOT_SAME_DEVICE:
      errno = EXDEV;
      break;

    case ERROR_WRITE_PROTECT:
      errno = EROFS;
      break;

    case ERROR_WRITE_FAULT:
    case ERROR_READ_FAULT:
    case ERROR_GEN_FAILURE:
      errno = EIO;
      break;

    case ERROR_HANDLE_DISK_FULL:
    case ERROR_DISK_FULL:
    case ERROR_DISK_TOO_FRAGMENTED:
      errno = ENOSPC;
      break;

    case ERROR_FILE_EXISTS:
    case ERROR_ALREADY_EXISTS:
      errno = EEXIST;
      break;

    case ERROR_BUFFER_OVERFLOW:
    case ERROR_FILENAME_EXCED_RANGE:
      errno = ENAMETOOLONG;
      break;

    case ERROR_INVALID_NAME:
    case ERROR_DELETE_PENDING:
      errno = EPERM;        /* ? */
      break;

# ifndef ERROR_FILE_TOO_LARGE
/* This value is documented but not defined in all versions of windows.h. */
#  define ERROR_FILE_TOO_LARGE 223
# endif
    case ERROR_FILE_TOO_LARGE:
      errno = EFBIG;
      break;

    default:
      errno = EINVAL;
      break;
    }

  return -1;
}

#else /* ! W32 platform */

# if RENAME_DEST_EXISTS_BUG
#  error Please report your platform and this message to bug-gnulib@gnu.org.
# elif RENAME_TRAILING_SLASH_BUG
#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>

#  include "dirname.h"
#  include "xalloc.h"

/* Rename the file SRC to DST, removing any trailing
   slashes from SRC.  Needed for SunOS 4.1.1_U1.  */

int
rpl_rename (char const *src, char const *dst)
{
  char *src_temp;
  int ret_val;
  size_t s_len = strlen (src);

  if (s_len && src[s_len - 1] == '/')
    {
      src_temp = xstrdup (src);
      strip_trailing_slashes (src_temp);
    }
  else
    src_temp = (char *) src;

  ret_val = rename (src_temp, dst);

  if (src_temp != src)
    free (src_temp);

  return ret_val;
}
# endif /* RENAME_TRAILING_SLASH_BUG */
#endif /* ! W32 platform */
