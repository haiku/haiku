/* Copyright (C) 1993-2014 Free Software Foundation, Inc.
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
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.

   As a special exception, if you link the code in this file with
   files compiled with a GNU compiler to produce an executable,
   that does not cause the resulting executable to be covered by
   the GNU Lesser General Public License.  This exception does not
   however invalidate any other reasons why the executable file
   might be covered by the GNU Lesser General Public License.
   This exception applies to code released by its copyright holders
   in files containing the exception.  */

#include <assert.h>
#include "strfile.h"
#include "libioP.h"
#include <string.h>
#include <stdio_ext.h>

void
_IO_str_init_static_internal (sf, ptr, size, pstart)
     _IO_strfile *sf;
     char *ptr;
     _IO_size_t size;
     char *pstart;
{
  _IO_FILE *fp = &sf->_sbf._f;
  char *end;

  if (size == 0)
    end = ptr + strlen (ptr);
  else if ((_IO_size_t) ptr + size > (_IO_size_t) ptr)
    end = ptr + size;
  else
    end = (char *) -1;
  _IO_setb (fp, ptr, end, 0);

  fp->_IO_write_base = ptr;
  fp->_IO_read_base = ptr;
  fp->_IO_read_ptr = ptr;
  if (pstart)
    {
      fp->_IO_write_ptr = pstart;
      fp->_IO_write_end = end;
      fp->_IO_read_end = pstart;
    }
  else
    {
      fp->_IO_write_ptr = ptr;
      fp->_IO_write_end = ptr;
      fp->_IO_read_end = end;
    }
  /* A null _allocate_buffer function flags the strfile as being static. */
  sf->_s._allocate_buffer = (_IO_alloc_type) 0;
}

void
_IO_str_init_static (sf, ptr, size, pstart)
     _IO_strfile *sf;
     char *ptr;
     int size;
     char *pstart;
{
  return _IO_str_init_static_internal (sf, ptr, size < 0 ? -1 : size, pstart);
}

void
_IO_str_init_readonly (sf, ptr, size)
     _IO_strfile *sf;
     const char *ptr;
     int size;
{
  _IO_str_init_static_internal (sf, (char *) ptr, size < 0 ? -1 : size, NULL);
  sf->_sbf._f._IO_file_flags |= _IO_NO_WRITES;
}

int
_IO_str_overflow (fp, c)
     _IO_FILE *fp;
     int c;
{
  int flush_only = c == EOF;
  _IO_size_t pos;
  if (fp->_flags & _IO_NO_WRITES)
      return flush_only ? 0 : EOF;
  if ((fp->_flags & _IO_TIED_PUT_GET) && !(fp->_flags & _IO_CURRENTLY_PUTTING))
    {
      fp->_flags |= _IO_CURRENTLY_PUTTING;
      fp->_IO_write_ptr = fp->_IO_read_ptr;
      fp->_IO_read_ptr = fp->_IO_read_end;
    }
  pos = fp->_IO_write_ptr - fp->_IO_write_base;
  if (pos >= (_IO_size_t) (_IO_blen (fp) + flush_only))
    {
      if (fp->_flags & _IO_USER_BUF) /* not allowed to enlarge */
	return EOF;
      else
	{
	  char *new_buf;
	  char *old_buf = fp->_IO_buf_base;
	  size_t old_blen = _IO_blen (fp);
	  _IO_size_t new_size = 2 * old_blen + 100;
	  if (new_size < old_blen)
	    return EOF;
	  new_buf
	    = (char *) (*((_IO_strfile *) fp)->_s._allocate_buffer) (new_size);
	  if (new_buf == NULL)
	    {
	      /*	  __ferror(fp) = 1; */
	      return EOF;
	    }
	  if (old_buf)
	    {
	      memcpy (new_buf, old_buf, old_blen);
	      (*((_IO_strfile *) fp)->_s._free_buffer) (old_buf);
	      /* Make sure _IO_setb won't try to delete _IO_buf_base. */
	      fp->_IO_buf_base = NULL;
	    }
	  memset (new_buf + old_blen, '\0', new_size - old_blen);

	  _IO_setb (fp, new_buf, new_buf + new_size, 1);
	  fp->_IO_read_base = new_buf + (fp->_IO_read_base - old_buf);
	  fp->_IO_read_ptr = new_buf + (fp->_IO_read_ptr - old_buf);
	  fp->_IO_read_end = new_buf + (fp->_IO_read_end - old_buf);
	  fp->_IO_write_ptr = new_buf + (fp->_IO_write_ptr - old_buf);

	  fp->_IO_write_base = new_buf;
	  fp->_IO_write_end = fp->_IO_buf_end;
	}
    }

  if (!flush_only)
    *fp->_IO_write_ptr++ = (unsigned char) c;
  if (fp->_IO_write_ptr > fp->_IO_read_end)
    fp->_IO_read_end = fp->_IO_write_ptr;
  return c;
}
libc_hidden_def (_IO_str_overflow)

int
_IO_str_underflow (fp)
     _IO_FILE *fp;
{
  if (fp->_IO_write_ptr > fp->_IO_read_end)
    fp->_IO_read_end = fp->_IO_write_ptr;
  if ((fp->_flags & _IO_TIED_PUT_GET) && (fp->_flags & _IO_CURRENTLY_PUTTING))
    {
      fp->_flags &= ~_IO_CURRENTLY_PUTTING;
      fp->_IO_read_ptr = fp->_IO_write_ptr;
      fp->_IO_write_ptr = fp->_IO_write_end;
    }
  if (fp->_IO_read_ptr < fp->_IO_read_end)
    return *((unsigned char *) fp->_IO_read_ptr);
  else
    return EOF;
}
libc_hidden_def (_IO_str_underflow)

/* The size of the valid part of the buffer.  */

_IO_ssize_t
_IO_str_count (fp)
     _IO_FILE *fp;
{
  return ((fp->_IO_write_ptr > fp->_IO_read_end
	   ? fp->_IO_write_ptr : fp->_IO_read_end)
	  - fp->_IO_read_base);
}


static int
enlarge_userbuf (_IO_FILE *fp, _IO_off64_t offset, int reading)
{
  _IO_ssize_t oldend, newsize;
  char *oldbuf, *newbuf;
  if ((_IO_ssize_t) offset <= _IO_blen (fp))
    return 0;

  oldend = fp->_IO_write_end - fp->_IO_write_base;

  /* Try to enlarge the buffer.  */
  if (fp->_flags & _IO_USER_BUF)
    /* User-provided buffer.  */
    return 1;

  newsize = offset + 100;
  oldbuf = fp->_IO_buf_base;
  newbuf
    = (char *) (*((_IO_strfile *) fp)->_s._allocate_buffer) (newsize);
  if (newbuf == NULL)
    return 1;

  if (oldbuf != NULL)
    {
      memcpy (newbuf, oldbuf, _IO_blen (fp));
      (*((_IO_strfile *) fp)->_s._free_buffer) (oldbuf);
      /* Make sure _IO_setb won't try to delete
	 _IO_buf_base. */
      fp->_IO_buf_base = NULL;
    }

  _IO_setb (fp, newbuf, newbuf + newsize, 1);

  if (reading)
    {
      fp->_IO_write_base = newbuf + (fp->_IO_write_base - oldbuf);
      fp->_IO_write_ptr = newbuf + (fp->_IO_write_ptr - oldbuf);
      fp->_IO_write_end = newbuf + (fp->_IO_write_end - oldbuf);
      fp->_IO_read_ptr = newbuf + (fp->_IO_read_ptr - oldbuf);

      fp->_IO_read_base = newbuf;
      fp->_IO_read_end = fp->_IO_buf_end;
    }
  else
    {
      fp->_IO_read_base = newbuf + (fp->_IO_read_base - oldbuf);
      fp->_IO_read_ptr = newbuf + (fp->_IO_read_ptr - oldbuf);
      fp->_IO_read_end = newbuf + (fp->_IO_read_end - oldbuf);
      fp->_IO_write_ptr = newbuf + (fp->_IO_write_ptr - oldbuf);

      fp->_IO_write_base = newbuf;
      fp->_IO_write_end = fp->_IO_buf_end;
    }

  /* Clear the area between the last write position and th
     new position.  */
  assert (offset >= oldend);
  if (reading)
    memset (fp->_IO_read_base + oldend, '\0', offset - oldend);
  else
    memset (fp->_IO_write_base + oldend, '\0', offset - oldend);

  return 0;
}


_IO_off64_t
_IO_str_seekoff (fp, offset, dir, mode)
     _IO_FILE *fp;
     _IO_off64_t offset;
     int dir;
     int mode;
{
  _IO_off64_t new_pos;

  if (mode == 0 && (fp->_flags & _IO_TIED_PUT_GET))
    mode = (fp->_flags & _IO_CURRENTLY_PUTTING ? _IOS_OUTPUT : _IOS_INPUT);

  if (mode == 0)
    {
      /* Don't move any pointers. But there is no clear indication what
	 mode FP is in. Let's guess. */
      if (fp->_IO_file_flags & _IO_NO_WRITES)
        new_pos = fp->_IO_read_ptr - fp->_IO_read_base;
      else
        new_pos = fp->_IO_write_ptr - fp->_IO_write_base;
    }
  else
    {
      _IO_ssize_t cur_size = _IO_str_count(fp);
      new_pos = EOF;

      /* Move the get pointer, if requested. */
      if (mode & _IOS_INPUT)
	{
	  switch (dir)
	    {
	    case _IO_seek_end:
	      offset += cur_size;
	      break;
	    case _IO_seek_cur:
	      offset += fp->_IO_read_ptr - fp->_IO_read_base;
	      break;
	    default: /* case _IO_seek_set: */
	      break;
	    }
	  if (offset < 0)
	    return EOF;
	  if ((_IO_ssize_t) offset > cur_size
	      && enlarge_userbuf (fp, offset, 1) != 0)
	    return EOF;
	  fp->_IO_read_ptr = fp->_IO_read_base + offset;
	  fp->_IO_read_end = fp->_IO_read_base + cur_size;
	  new_pos = offset;
	}

      /* Move the put pointer, if requested. */
      if (mode & _IOS_OUTPUT)
	{
	  switch (dir)
	    {
	    case _IO_seek_end:
	      offset += cur_size;
	      break;
	    case _IO_seek_cur:
	      offset += fp->_IO_write_ptr - fp->_IO_write_base;
	      break;
	    default: /* case _IO_seek_set: */
	      break;
	    }
	  if (offset < 0)
	    return EOF;
	  if ((_IO_ssize_t) offset > cur_size
	      && enlarge_userbuf (fp, offset, 0) != 0)
	    return EOF;
	  fp->_IO_write_ptr = fp->_IO_write_base + offset;
	  new_pos = offset;
	}
    }
  return new_pos;
}
libc_hidden_def (_IO_str_seekoff)

int
_IO_str_pbackfail (fp, c)
     _IO_FILE *fp;
     int c;
{
  if ((fp->_flags & _IO_NO_WRITES) && c != EOF)
    return EOF;
  return _IO_default_pbackfail (fp, c);
}
libc_hidden_def (_IO_str_pbackfail)

void
_IO_str_finish (fp, dummy)
     _IO_FILE *fp;
     int dummy;
{
  if (fp->_IO_buf_base && !(fp->_flags & _IO_USER_BUF))
    (((_IO_strfile *) fp)->_s._free_buffer) (fp->_IO_buf_base);
  fp->_IO_buf_base = NULL;

  _IO_default_finish (fp, 0);
}

const struct _IO_jump_t _IO_str_jumps =
{
  JUMP_INIT_DUMMY,
  JUMP_INIT(finish, _IO_str_finish),
  JUMP_INIT(overflow, _IO_str_overflow),
  JUMP_INIT(underflow, _IO_str_underflow),
  JUMP_INIT(uflow, _IO_default_uflow),
  JUMP_INIT(pbackfail, _IO_str_pbackfail),
  JUMP_INIT(xsputn, _IO_default_xsputn),
  JUMP_INIT(xsgetn, _IO_default_xsgetn),
  JUMP_INIT(seekoff, _IO_str_seekoff),
  JUMP_INIT(seekpos, _IO_default_seekpos),
  JUMP_INIT(setbuf, _IO_default_setbuf),
  JUMP_INIT(sync, _IO_default_sync),
  JUMP_INIT(doallocate, _IO_default_doallocate),
  JUMP_INIT(read, _IO_default_read),
  JUMP_INIT(write, _IO_default_write),
  JUMP_INIT(seek, _IO_default_seek),
  JUMP_INIT(close, _IO_default_close),
  JUMP_INIT(stat, _IO_default_stat),
  JUMP_INIT(showmanyc, _IO_default_showmanyc),
  JUMP_INIT(imbue, _IO_default_imbue)
};
