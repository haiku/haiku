/* Copyright (C) 1994,1997,1999,2000,2001,2002 Free Software Foundation, Inc.
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
#include "strfile.h"


typedef struct
{
  _IO_strfile f;
  /* This is used for the characters which do not fit in the buffer
     provided by the user.  */
  wchar_t overflow_buf[64];
} _IO_strnfile;


static wint_t _IO_wstrn_overflow __P ((_IO_FILE *fp, wint_t c));

static wint_t
_IO_wstrn_overflow (fp, c)
     _IO_FILE *fp;
     wint_t c;
{
  /* When we come to here this means the user supplied buffer is
     filled.  But since we must return the number of characters which
     would have been written in total we must provide a buffer for
     further use.  We can do this by writing on and on in the overflow
     buffer in the _IO_strnfile structure.  */
  _IO_strnfile *snf = (_IO_strnfile *) fp;

  if (fp->_wide_data->_IO_buf_base != snf->overflow_buf)
    {
      INTUSE(_IO_wsetb) (fp, snf->overflow_buf,
			 snf->overflow_buf + (sizeof (snf->overflow_buf)
					      / sizeof (wchar_t)), 0);

      fp->_wide_data->_IO_write_base = snf->overflow_buf;
      fp->_wide_data->_IO_read_base = snf->overflow_buf;
      fp->_wide_data->_IO_read_ptr = snf->overflow_buf;
      fp->_wide_data->_IO_read_end = (snf->overflow_buf
				      + (sizeof (snf->overflow_buf)
					 / sizeof (wchar_t)));
    }

  fp->_wide_data->_IO_write_ptr = snf->overflow_buf;
  fp->_wide_data->_IO_write_end = snf->overflow_buf;

  /* Since we are not really interested in storing the characters
     which do not fit in the buffer we simply ignore it.  */
  return c;
}


static struct _IO_jump_t _IO_wstrn_jumps =
{
  JUMP_INIT_DUMMY,
  JUMP_INIT(finish, _IO_wstr_finish),
  JUMP_INIT(overflow, (_IO_overflow_t) _IO_wstrn_overflow),
  JUMP_INIT(underflow, (_IO_underflow_t) _IO_wstr_underflow),
  JUMP_INIT(uflow, (_IO_underflow_t) INTUSE(_IO_wdefault_uflow)),
  JUMP_INIT(pbackfail, (_IO_pbackfail_t) _IO_wstr_pbackfail),
  JUMP_INIT(xsputn, INTUSE(_IO_wdefault_xsputn)),
  JUMP_INIT(xsgetn, INTUSE(_IO_wdefault_xsgetn)),
  JUMP_INIT(seekoff, _IO_wstr_seekoff),
  JUMP_INIT(seekpos, _IO_default_seekpos),
  JUMP_INIT(setbuf, _IO_default_setbuf),
  JUMP_INIT(sync, _IO_default_sync),
  JUMP_INIT(doallocate, INTUSE(_IO_wdefault_doallocate)),
  JUMP_INIT(read, _IO_default_read),
  JUMP_INIT(write, _IO_default_write),
  JUMP_INIT(seek, _IO_default_seek),
  JUMP_INIT(close, _IO_default_close),
  JUMP_INIT(stat, _IO_default_stat),
  JUMP_INIT(showmanyc, _IO_default_showmanyc),
  JUMP_INIT(imbue, _IO_default_imbue)
};


int
_IO_vswprintf (string, maxlen, format, args)
     wchar_t *string;
     _IO_size_t maxlen;
     const wchar_t *format;
     _IO_va_list args;
{
  _IO_strnfile sf;
  int ret;
  struct _IO_wide_data wd;
#ifdef _IO_MTSAFE_IO
  sf.f._sbf._f._lock = NULL;
#endif

  if (maxlen == 0)
    /* Since we have to write at least the terminating L'\0' a buffer
       length of zero always makes the function fail.  */
    return -1;

  _IO_no_init (&sf.f._sbf._f, _IO_USER_LOCK, 0, &wd, &_IO_wstrn_jumps);
  _IO_fwide (&sf.f._sbf._f, 1);
  string[0] = L'\0';
  _IO_wstr_init_static (&sf.f._sbf._f, string, maxlen - 1, string);
  ret = _IO_vfwprintf ((_IO_FILE *) &sf.f._sbf, format, args);

  if (sf.f._sbf._f._wide_data->_IO_buf_base == sf.overflow_buf)
    /* ISO C99 requires swprintf/vswprintf to return an error if the
       output does not fit int he provided buffer.  */
    return -1;

  /* Terminate the string.  */
  *sf.f._sbf._f._wide_data->_IO_write_ptr = '\0';

  return ret;
}

#ifdef weak_alias
weak_alias (_IO_vswprintf, __vswprintf)
weak_alias (_IO_vswprintf, vswprintf)
#endif
