/* Copyright (C) 1993, 1995-2000, 2001, 2002 Free Software Foundation, Inc.
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
#include <stdlib.h>
#include <errno.h>
/* ANSI explicily requires setting errno to a positive value on failure. */

long int
_IO_ftell (fp)
     _IO_FILE *fp;
{
  _IO_off64_t pos;
  CHECK_FILE (fp, -1L);
  _IO_cleanup_region_start ((void (*) __P ((void *))) _IO_funlockfile, fp);
  _IO_flockfile (fp);
  pos = _IO_seekoff_unlocked (fp, 0, _IO_seek_cur, 0);
  if (_IO_in_backup (fp))
    {
      if (fp->_vtable_offset != 0 || fp->_mode <= 0)
	pos -= fp->_IO_save_end - fp->_IO_save_base;
    }
  _IO_funlockfile (fp);
  _IO_cleanup_region_end (0);
  if (pos == _IO_pos_BAD)
    {
#ifdef EIO
      if (errno == 0)
	__set_errno (EIO);
#endif
      return -1L;
    }
  if ((_IO_off64_t) (off_t) pos != pos)
    {
#ifdef EOVERFLOW
      __set_errno (EOVERFLOW);
#endif
      return -1L;
    }
  return pos;
}
INTDEF(_IO_ftell)

#ifdef weak_alias
weak_alias (_IO_ftell, ftell)
#endif
