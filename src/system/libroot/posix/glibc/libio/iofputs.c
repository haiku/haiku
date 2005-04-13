/* Copyright (C) 1993,1996,1997,1998,1999,2002 Free Software Foundation, Inc.
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
#include <string.h>

int
_IO_fputs (str, fp)
      const char *str;
      _IO_FILE *fp;
{
  _IO_size_t len = strlen (str);
  int result = EOF;

  CHECK_FILE (fp, EOF);
  _IO_cleanup_region_start ((void (*) __P ((void *))) _IO_funlockfile, fp);
  _IO_flockfile (fp);
  if ((fp->_vtable_offset != 0 || _IO_fwide (fp, -1) == -1)
      && _IO_sputn (fp, str, len) == len)
    result = 1;
  _IO_funlockfile (fp);
  _IO_cleanup_region_end (0);
  return result;
}
libc_hidden_def (_IO_fputs)

#ifdef weak_alias
weak_alias (_IO_fputs, fputs)

# ifndef _IO_MTSAFE_IO
weak_alias (_IO_fputs, fputs_unlocked)
libc_hidden_ver (_IO_fputs, fputs_unlocked)
# endif
#endif
