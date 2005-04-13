/* Copyright (C) 1993,1997,1998,1999,2000,2002 Free Software Foundation, Inc.
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
#include <wchar.h>

#ifdef _LIBC
# define wmemcpy __wmemcpy
#endif

#if defined _LIBC || !_G_HAVE_IO_GETLINE_INFO

_IO_size_t
_IO_getwline (fp, buf, n, delim, extract_delim)
     _IO_FILE *fp;
     wchar_t *buf;
     _IO_size_t n;
     wint_t delim;
     int extract_delim;
{
  return _IO_getwline_info (fp, buf, n, delim, extract_delim, (wint_t *) 0);
}

/* Algorithm based on that used by Berkeley pre-4.4 fgets implementation.

   Read chars into buf (of size n), until delim is seen.
   Return number of chars read (at most n).
   Does not put a terminating '\0' in buf.
   If extract_delim < 0, leave delimiter unread.
   If extract_delim > 0, insert delim in output. */

_IO_size_t
_IO_getwline_info (fp, buf, n, delim, extract_delim, eof)
     _IO_FILE *fp;
     wchar_t *buf;
     _IO_size_t n;
     wint_t delim;
     int extract_delim;
     wint_t *eof;
{
  wchar_t *ptr = buf;
  if (eof != NULL)
    *eof = 0;
  if (__builtin_expect (fp->_mode, 1) == 0)
    _IO_fwide (fp, 1);
  while (n != 0)
    {
      _IO_ssize_t len = (fp->_wide_data->_IO_read_end
			 - fp->_wide_data->_IO_read_ptr);
      if (len <= 0)
	{
	  wint_t wc = __wuflow (fp);
	  if (wc == WEOF)
	    {
	      if (eof)
		*eof = wc;
	      break;
	    }
	  if (wc == delim)
	    {
 	      if (extract_delim > 0)
		*ptr++ = wc;
	      else if (extract_delim < 0)
		INTUSE(_IO_sputbackc) (fp, wc);
	      return ptr - buf;
	      if (extract_delim > 0)
		++len;
	    }
	  *ptr++ = wc;
	  n--;
	}
      else
	{
	  wchar_t *t;
	  if ((_IO_size_t) len >= n)
	    len = n;
	  t = wmemchr ((void *) fp->_wide_data->_IO_read_ptr, delim, len);
	  if (t != NULL)
	    {
	      _IO_size_t old_len = ptr - buf;
	      len = t - fp->_wide_data->_IO_read_ptr;
	      if (extract_delim >= 0)
		{
		  ++t;
		  if (extract_delim > 0)
		    ++len;
		}
	      wmemcpy ((void *) ptr, (void *) fp->_wide_data->_IO_read_ptr,
		       len);
	      fp->_wide_data->_IO_read_ptr = t;
	      return old_len + len;
	    }
	  wmemcpy ((void *) ptr, (void *) fp->_wide_data->_IO_read_ptr, len);
	  fp->_wide_data->_IO_read_ptr += len;
	  ptr += len;
	  n -= len;
	}
    }
  return ptr - buf;
}

#endif /* Defined _LIBC || !_G_HAVE_IO_GETLINE_INFO */
