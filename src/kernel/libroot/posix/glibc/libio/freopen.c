/* Copyright (C) 1993,95,96,97,98,2000,2001,2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.

   As a special exception, if you link the code in this file with
   files compiled with a GNU compiler to produce an executable,
   that does not cause the resulting executable to be covered by
   the GNU Lesser General Public License.  This exception does not
   however invalidate any other reasons why the executable file
   might be covered by the GNU Lesser General Public License.
   This exception applies to code released by its copyright holders
   in files containing the exception.  */

#include "libioP.h"
#include "stdio.h"

#include <shlib-compat.h>

FILE*
freopen (filename, mode, fp)
     const char* filename;
     const char* mode;
     FILE* fp;
{
  FILE *result;
  CHECK_FILE (fp, NULL);
  if (!(fp->_flags & _IO_IS_FILEBUF))
    return NULL;
  _IO_cleanup_region_start ((void (*) __P ((void *))) _IO_funlockfile, fp);
  _IO_flockfile (fp);
#if SHLIB_COMPAT (libc, GLIBC_2_0, GLIBC_2_1)
  if (&_IO_stdin_used == NULL)
    /* If the shared C library is used by the application binary which
       was linked against the older version of libio, we just use the
       older one even for internal use to avoid trouble since a pointer
       to the old libio may be passed into shared C library and wind
       up here. */
    result = _IO_old_freopen (filename, mode, fp);
  else
#endif
    result = _IO_freopen (filename, mode, fp);
  if (result != NULL)
    /* unbound stream orientation */
    result->_mode = 0;
  _IO_funlockfile (fp);
  _IO_cleanup_region_end (0);
  return result;
}

#if 0
#include <fd_to_filename.h>

FILE*
freopen (filename, mode, fp)
     const char* filename;
     const char* mode;
     FILE* fp;
{
  FILE *result;
  int fd = -1;
  CHECK_FILE (fp, NULL);
  if (!(fp->_flags & _IO_IS_FILEBUF))
    return NULL;
  _IO_cleanup_region_start ((void (*) __P ((void *))) _IO_funlockfile, fp);
  _IO_flockfile (fp);
  if (filename == NULL && _IO_fileno (fp) >= 0)
    {
      fd = __dup (_IO_fileno (fp));
      if (fd != -1)
	filename = fd_to_filename (fd);
    }
#if SHLIB_COMPAT (libc, GLIBC_2_0, GLIBC_2_1)
  if (&_IO_stdin_used == NULL)
    {
      /* If the shared C library is used by the application binary which
	 was linked against the older version of libio, we just use the
	 older one even for internal use to avoid trouble since a pointer
	 to the old libio may be passed into shared C library and wind
	 up here. */
      _IO_old_file_close_it (fp);
      _IO_JUMPS ((struct _IO_FILE_plus *) fp) = &_IO_old_file_jumps;
      result = _IO_old_file_fopen (fp, filename, mode);
    }
  else
#endif
    {
      INTUSE(_IO_file_close_it) (fp);
      _IO_JUMPS ((struct _IO_FILE_plus *) fp) = &INTUSE(_IO_file_jumps);
//      if (fp->_vtable_offset == 0 && fp->_wide_data != NULL)
//	fp->_wide_data->_wide_vtable = &INTUSE(_IO_wfile_jumps);
      result = INTUSE(_IO_file_fopen) (fp, filename, mode, 1);
      if (result != NULL)
	result = __fopen_maybe_mmap (result);
    }
  if (result != NULL)
    /* unbound stream orientation */
    result->_mode = 0;
  if (fd != -1)
    {
      __close (fd);
      if (filename != NULL)
	free ((char *) filename);
    }
  _IO_funlockfile (fp);
  _IO_cleanup_region_end (0);
  return result;
}
#endif
