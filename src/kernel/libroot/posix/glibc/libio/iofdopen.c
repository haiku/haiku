/* Copyright (C) 1993,1994,1997-1999,2000,2002 Free Software Foundation, Inc.
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
   in files containing the exception.
*/


#include "libioP.h"
#include <stdlib.h>
#include <fcntl.h>

#ifdef _LIBC
# include <shlib-compat.h>
#endif


_IO_FILE *
_IO_new_fdopen (int fd, const char *mode)
{
  int read_write;
  int posix_mode = 0;
  struct locked_FILE
  {
    struct _IO_FILE_plus fp;
#ifdef _IO_MTSAFE_IO
    _IO_lock_t lock;
#endif
    struct _IO_wide_data wd;
  } *new_f;
  int fd_flags;
  int i;
  int use_mmap = 0;

  switch (*mode)
    {
    case 'r':
      read_write = _IO_NO_WRITES;
      break;
    case 'w':
      read_write = _IO_NO_READS;
      break;
    case 'a':
      posix_mode = O_APPEND;
      read_write = _IO_NO_READS|_IO_IS_APPENDING;
      break;
    default:
      MAYBE_SET_EINVAL;
      return NULL;
  }
  for (i = 1; i < 5; ++i)
    {
      switch (*++mode)
	{
	case '\0':
	  break;
	case '+':
	  read_write &= _IO_IS_APPENDING;
	  break;
	case 'm':
	  use_mmap = 1;
	  continue;
	case 'x':
	case 'b':
	default:
	  /* Ignore */
	  continue;
	}
      break;
    }
#ifdef F_GETFL
  fd_flags = fcntl(fd, F_GETFL);
#ifndef O_ACCMODE
#define O_ACCMODE (O_RDONLY|O_WRONLY|O_RDWR)
#endif
  if (fd_flags == -1)
    return NULL;

  if (((fd_flags & O_ACCMODE) == O_RDONLY && !(read_write & _IO_NO_WRITES))
      || ((fd_flags & O_ACCMODE) == O_WRONLY && !(read_write & _IO_NO_READS)))
    {
      MAYBE_SET_EINVAL;
      return NULL;
    }

  /* The May 93 draft of P1003.4/D14.1 (redesignated as 1003.1b)
     [System Application Program Interface (API) Amendment 1:
     Realtime Extensions], Rationale B.8.3.3
     Open a Stream on a File Descriptor says:

         Although not explicitly required by POSIX.1, a good
         implementation of append ("a") mode would cause the
         O_APPEND flag to be set.

     (Historical implementations [such as Solaris2] do a one-time
     seek in fdopen.)

     However, we do not turn O_APPEND off if the mode is "w" (even
     though that would seem consistent) because that would be more
     likely to break historical programs.
     */
  if ((posix_mode & O_APPEND) && !(fd_flags & O_APPEND))
    {
#ifdef F_SETFL
      if (fcntl(fd, F_SETFL, fd_flags | O_APPEND) == -1)
#endif
	return NULL;
    }
#endif

  new_f = (struct locked_FILE *) malloc (sizeof (struct locked_FILE));
  if (new_f == NULL)
    return NULL;
#ifdef _IO_MTSAFE_IO
  new_f->fp.file._lock = &new_f->lock;
#endif
  /* Set up initially to use the `maybe_mmap' jump tables rather than using
     __fopen_maybe_mmap to do it, because we need them in place before we
     call _IO_file_attach or else it will allocate a buffer immediately.  */
  _IO_no_init (&new_f->fp.file, 0, 0, &new_f->wd,
#ifdef _G_HAVE_MMAP
	       (use_mmap && (read_write & _IO_NO_WRITES))
	       ? &_IO_wfile_jumps_maybe_mmap :
#endif
	       &INTUSE(_IO_wfile_jumps));
  _IO_JUMPS (&new_f->fp) =
#ifdef _G_HAVE_MMAP
    (use_mmap && (read_write & _IO_NO_WRITES)) ? &_IO_file_jumps_maybe_mmap :
#endif
      &INTUSE(_IO_file_jumps);
  INTUSE(_IO_file_init) (&new_f->fp);
#if  !_IO_UNIFIED_JUMPTABLES
  new_f->fp.vtable = NULL;
#endif
  if (INTUSE(_IO_file_attach) ((_IO_FILE *) &new_f->fp, fd) == NULL)
    {
      INTUSE(_IO_setb) (&new_f->fp.file, NULL, NULL, 0);
      INTUSE(_IO_un_link) (&new_f->fp);
      free (new_f);
      return NULL;
    }
  new_f->fp.file._flags &= ~_IO_DELETE_DONT_CLOSE;

  new_f->fp.file._IO_file_flags =
    _IO_mask_flags (&new_f->fp.file, read_write,
		    _IO_NO_READS+_IO_NO_WRITES+_IO_IS_APPENDING);

  return &new_f->fp.file;
}
INTDEF2(_IO_new_fdopen, _IO_fdopen)

strong_alias (_IO_new_fdopen, __new_fdopen)
versioned_symbol (libc, _IO_new_fdopen, _IO_fdopen, GLIBC_2_1);
versioned_symbol (libc, __new_fdopen, fdopen, GLIBC_2_1);
