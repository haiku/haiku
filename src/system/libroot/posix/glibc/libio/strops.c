/* Copyright (C) 1993, 1997-2000, 2001, 2002 Free Software Foundation, Inc.
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
#include <string.h>
#include <stdio_ext.h>

#if 0
/* The following definitions are for exposition only.
   They map the terminology used in the ANSI/ISO C++ draft standard
   to the implementation. */

/* allocated:  set  when a dynamic array object has been allocated, and
   hence should be freed by the destructor for the strstreambuf object. */
#define ALLOCATED(FP) ((FP)->_f._IO_buf_base && DYNAMIC(FP))

/* constant:  set when the array object has const elements,
   so the output sequence cannot be written. */
#define CONSTANT(FP) ((FP)->_f._IO_file_flags & _IO_NO_WRITES)

/* alsize:  the suggested minimum size for a dynamic array object. */
#define ALSIZE(FP) ??? /* not stored */

/* palloc: points to the function to call to allocate a dynamic array object.*/
#define PALLOC(FP) \
  ((FP)->_s._allocate_buffer == default_alloc ? 0 : (FP)->_s._allocate_buffer)

/* pfree: points  to  the  function  to call to free a dynamic array object. */
#define PFREE(FP) \
  ((FP)->_s._free_buffer == default_free ? 0 : (FP)->_s._free_buffer)

#endif

#ifdef TODO
/* An "unbounded buffer" is when a buffer is supplied, but with no
   specified length.  An example is the buffer argument to sprintf.
   */
#endif

void
_IO_str_init_static (sf, ptr, size, pstart)
     _IO_strfile *sf;
     char *ptr;
     int size;
     char *pstart;
{
  _IO_FILE *fp = &sf->_sbf._f;

  if (size == 0)
    size = strlen (ptr);
  else if (size < 0)
    {
      /* If size is negative 'the characters are assumed to
	 continue indefinitely.'  This is kind of messy ... */
      int s;
      size = 512;
      /* Try increasing powers of 2, as long as we don't wrap around. */
      for (; s = 2*size, s > 0 && ptr + s > ptr && s < 0x4000000L; )
	size = s;
      /* Try increasing size as much as we can without wrapping around. */
      for (s = size >> 1; s > 0; s >>= 1)
	{
	  if (ptr + size + s > ptr)
	    size += s;
	}
    }
  INTUSE(_IO_setb) (fp, ptr, ptr + size, 0);

  fp->_IO_write_base = ptr;
  fp->_IO_read_base = ptr;
  fp->_IO_read_ptr = ptr;
  if (pstart)
    {
      fp->_IO_write_ptr = pstart;
      fp->_IO_write_end = ptr + size;
      fp->_IO_read_end = pstart;
    }
  else
    {
      fp->_IO_write_ptr = ptr;
      fp->_IO_write_end = ptr;
      fp->_IO_read_end = ptr+size;
    }
  /* A null _allocate_buffer function flags the strfile as being static. */
  sf->_s._allocate_buffer = (_IO_alloc_type) 0;
}
INTDEF(_IO_str_init_static)

void
_IO_str_init_readonly (sf, ptr, size)
     _IO_strfile *sf;
     const char *ptr;
     int size;
{
  INTUSE(_IO_str_init_static) (sf, (char *) ptr, size, NULL);
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
  pos =  fp->_IO_write_ptr - fp->_IO_write_base;
  if (pos >= (_IO_size_t) (_IO_blen (fp) + flush_only))
    {
      if (fp->_flags & _IO_USER_BUF) /* not allowed to enlarge */
	return EOF;
      else
	{
	  char *new_buf;
	  char *old_buf = fp->_IO_buf_base;
	  _IO_size_t new_size = 2 * _IO_blen (fp) + 100;
	  new_buf
	    = (char *) (*((_IO_strfile *) fp)->_s._allocate_buffer) (new_size);
	  if (new_buf == NULL)
	    {
	      /*	  __ferror(fp) = 1; */
	      return EOF;
	    }
	  if (old_buf)
	    {
	      memcpy (new_buf, old_buf, _IO_blen (fp));
	      (*((_IO_strfile *) fp)->_s._free_buffer) (old_buf);
	      /* Make sure _IO_setb won't try to delete _IO_buf_base. */
	      fp->_IO_buf_base = NULL;
	    }
#if 0
	  if (lenp == &LEN(fp)) /* use '\0'-filling */
	      memset (new_buf + pos, 0, blen() - pos);
#endif
	  INTUSE(_IO_setb) (fp, new_buf, new_buf + new_size, 1);
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
INTDEF(_IO_str_overflow)

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
INTDEF(_IO_str_underflow)

/* The size of the valid part of the buffer.  */

_IO_ssize_t
_IO_str_count (fp)
     _IO_FILE *fp;
{
  return ((fp->_IO_write_ptr > fp->_IO_read_end
	   ? fp->_IO_write_ptr : fp->_IO_read_end)
	  - fp->_IO_read_base);
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
	  if (offset < 0 || (_IO_ssize_t) offset > cur_size)
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
	  if (offset < 0 || (_IO_ssize_t) offset > cur_size)
	    return EOF;
	  fp->_IO_write_ptr = fp->_IO_write_base + offset;
	  new_pos = offset;
	}
    }
  return new_pos;
}
INTDEF(_IO_str_seekoff)

int
_IO_str_pbackfail (fp, c)
     _IO_FILE *fp;
     int c;
{
  if ((fp->_flags & _IO_NO_WRITES) && c != EOF)
    return EOF;
  return INTUSE(_IO_default_pbackfail) (fp, c);
}
INTDEF(_IO_str_pbackfail)

void
_IO_str_finish (fp, dummy)
     _IO_FILE *fp;
     int dummy;
{
  if (fp->_IO_buf_base && !(fp->_flags & _IO_USER_BUF))
    (((_IO_strfile *) fp)->_s._free_buffer) (fp->_IO_buf_base);
  fp->_IO_buf_base = NULL;

  INTUSE(_IO_default_finish) (fp, 0);
}

struct _IO_jump_t _IO_str_jumps =
{
  JUMP_INIT_DUMMY,
  JUMP_INIT(finish, _IO_str_finish),
  JUMP_INIT(overflow, INTUSE(_IO_str_overflow)),
  JUMP_INIT(underflow, INTUSE(_IO_str_underflow)),
  JUMP_INIT(uflow, INTUSE(_IO_default_uflow)),
  JUMP_INIT(pbackfail, INTUSE(_IO_str_pbackfail)),
  JUMP_INIT(xsputn, INTUSE(_IO_default_xsputn)),
  JUMP_INIT(xsgetn, INTUSE(_IO_default_xsgetn)),
  JUMP_INIT(seekoff, INTUSE(_IO_str_seekoff)),
  JUMP_INIT(seekpos, _IO_default_seekpos),
  JUMP_INIT(setbuf, _IO_default_setbuf),
  JUMP_INIT(sync, _IO_default_sync),
  JUMP_INIT(doallocate, INTUSE(_IO_default_doallocate)),
  JUMP_INIT(read, _IO_default_read),
  JUMP_INIT(write, _IO_default_write),
  JUMP_INIT(seek, _IO_default_seek),
  JUMP_INIT(close, _IO_default_close),
  JUMP_INIT(stat, _IO_default_stat),
  JUMP_INIT(showmanyc, _IO_default_showmanyc),
  JUMP_INIT(imbue, _IO_default_imbue)
};
