/* Copyright (C) 1993,94,97,99,2000,2002 Free Software Foundation, Inc.
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


#include <shlib-compat.h>
#if SHLIB_COMPAT (libc, GLIBC_2_0, GLIBC_2_1)

#define _IO_USE_OLD_IO_FILE
#include <stdlib.h>
#include "libioP.h"
#include <fcntl.h>


_IO_FILE *
_IO_old_fdopen (fd, mode)
     int fd;
     const char *mode;
{
  int read_write;
  int posix_mode = 0;
  struct locked_FILE
  {
    struct _IO_FILE_plus fp;
#ifdef _IO_MTSAFE_IO
    _IO_lock_t lock;
#endif
  } *new_f;
  int fd_flags;

  switch (*mode++)
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
  if (mode[0] == '+' || (mode[0] == 'b' && mode[1] == '+'))
    read_write &= _IO_IS_APPENDING;
#ifdef F_GETFL
  fd_flags = fcntl(fd, F_GETFL);
#ifndef O_ACCMODE
#define O_ACCMODE (O_RDONLY|O_WRONLY|O_RDWR)
#endif
  if (fd_flags == -1
      || ((fd_flags & O_ACCMODE) == O_RDONLY && !(read_write & _IO_NO_WRITES))
      || ((fd_flags & O_ACCMODE) == O_WRONLY && !(read_write & _IO_NO_READS)))
    return NULL;

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
  INTUSE(_IO_init) (&new_f->fp.file, 0);
  _IO_JUMPS (&new_f->fp) = &_IO_old_file_jumps;
  _IO_old_file_init (&new_f->fp);
#if  !_IO_UNIFIED_JUMPTABLES
  new_f->fp.vtable = NULL;
#endif
  if (_IO_old_file_attach (&new_f->fp.file, fd) == NULL)
    {
      INTUSE(_IO_un_link) (&new_f->fp);
      free (new_f);
      return NULL;
    }
  new_f->fp.file._flags &= ~_IO_DELETE_DONT_CLOSE;

  new_f->fp.file._IO_file_flags =
    _IO_mask_flags (&new_f->fp.file, read_write,
		    _IO_NO_READS+_IO_NO_WRITES+_IO_IS_APPENDING);

  return (_IO_FILE *) &new_f->fp;
}

strong_alias (_IO_old_fdopen, __old_fdopen)
compat_symbol (libc, _IO_old_fdopen, _IO_fdopen, GLIBC_2_0);
compat_symbol (libc, __old_fdopen, fdopen, GLIBC_2_0);

#endif
