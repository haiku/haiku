/* Copyright (C) 1993,1997,1998,1999,2001,2002 Free Software Foundation, Inc.
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

#include <libioP.h>
#include <stdlib.h>
#include <errno.h>

_IO_off64_t
_IO_seekoff_unlocked (fp, offset, dir, mode)
     _IO_FILE *fp;
     _IO_off64_t offset;
     int dir;
     int mode;
{
  if (dir != _IO_seek_cur && dir != _IO_seek_set && dir != _IO_seek_end)
    {
      __set_errno (EINVAL);
      return EOF;
    }

  /* If we have a backup buffer, get rid of it, since the __seekoff
     callback may not know to do the right thing about it.
     This may be over-kill, but it'll do for now. TODO */
  if (mode != 0 && ((_IO_fwide (fp, 0) < 0 && _IO_have_backup (fp))
		    || (_IO_fwide (fp, 0) > 0 && _IO_have_wbackup (fp))))
    {
      if (dir == _IO_seek_cur && _IO_in_backup (fp))
	{
	  if (fp->_vtable_offset != 0 || fp->_mode <= 0)
	    offset -= fp->_IO_read_end - fp->_IO_read_ptr;
	  else
	    abort ();
	}
      if (_IO_fwide (fp, 0) < 0)
	INTUSE(_IO_free_backup_area) (fp);
      else
	INTUSE(_IO_free_wbackup_area) (fp);
    }

  return _IO_SEEKOFF (fp, offset, dir, mode);
}


_IO_off64_t
_IO_seekoff (fp, offset, dir, mode)
     _IO_FILE *fp;
     _IO_off64_t offset;
     int dir;
     int mode;
{
  _IO_off64_t retval;

  _IO_cleanup_region_start ((void (*) __P ((void *))) _IO_funlockfile, fp);
  _IO_flockfile (fp);

  retval = _IO_seekoff_unlocked (fp, offset, dir, mode);

  _IO_funlockfile (fp);
  _IO_cleanup_region_end (0);
  return retval;
}
