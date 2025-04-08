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

#include <libioP.h>
#include <stdio.h>
#include <stdlib.h>
#include <shlib-compat.h>

/* Prototyped for local functions.  */
static _IO_ssize_t _IO_cookie_read (_IO_FILE* fp, void* buf,
				    _IO_ssize_t size);
static _IO_ssize_t _IO_cookie_write (_IO_FILE* fp,
				     const void* buf, _IO_ssize_t size);
static _IO_off64_t _IO_cookie_seek (_IO_FILE *fp, _IO_off64_t offset, int dir);
static _IO_off64_t _IO_cookie_seekoff (_IO_FILE *fp, _IO_off64_t offset,
				       int dir, int mode);
static int _IO_cookie_close (_IO_FILE* fp);

static _IO_ssize_t
_IO_cookie_read (fp, buf, size)
     _IO_FILE *fp;
     void *buf;
     _IO_ssize_t size;
{
  struct _IO_cookie_file *cfile = (struct _IO_cookie_file *) fp;

  if (cfile->__io_functions.read == NULL)
    return -1;

  return cfile->__io_functions.read (cfile->__cookie, buf, size);
}

static _IO_ssize_t
_IO_cookie_write (fp, buf, size)
     _IO_FILE *fp;
     const void *buf;
     _IO_ssize_t size;
{
  struct _IO_cookie_file *cfile = (struct _IO_cookie_file *) fp;
  _IO_ssize_t n;

  if (cfile->__io_functions.write == NULL)
    {
      fp->_flags |= _IO_ERR_SEEN;
      return 0;
    }

  n = cfile->__io_functions.write (cfile->__cookie, buf, size);
  if (n < size)
    fp->_flags |= _IO_ERR_SEEN;

  return n;
}

static _IO_off64_t
_IO_cookie_seek (fp, offset, dir)
     _IO_FILE *fp;
     _IO_off64_t offset;
     int dir;
{
  struct _IO_cookie_file *cfile = (struct _IO_cookie_file *) fp;

  return ((cfile->__io_functions.seek == NULL
	   || (cfile->__io_functions.seek (cfile->__cookie, &offset, dir)
	       == -1)
	   || offset == (_IO_off64_t) -1)
	  ? _IO_pos_BAD : offset);
}

static int
_IO_cookie_close (fp)
     _IO_FILE *fp;
{
  struct _IO_cookie_file *cfile = (struct _IO_cookie_file *) fp;

  if (cfile->__io_functions.close == NULL)
    return 0;

  return cfile->__io_functions.close (cfile->__cookie);
}


static _IO_off64_t
_IO_cookie_seekoff (fp, offset, dir, mode)
     _IO_FILE *fp;
     _IO_off64_t offset;
     int dir;
     int mode;
{
  /* We must force the fileops code to always use seek to determine
     the position.  */
  fp->_offset = _IO_pos_BAD;
  return _IO_file_seekoff (fp, offset, dir, mode);
}


static const struct _IO_jump_t _IO_cookie_jumps = {
  JUMP_INIT_DUMMY,
  JUMP_INIT(finish, _IO_file_finish),
  JUMP_INIT(overflow, _IO_file_overflow),
  JUMP_INIT(underflow, _IO_file_underflow),
  JUMP_INIT(uflow, _IO_default_uflow),
  JUMP_INIT(pbackfail, _IO_default_pbackfail),
  JUMP_INIT(xsputn, _IO_file_xsputn),
  JUMP_INIT(xsgetn, _IO_default_xsgetn),
  JUMP_INIT(seekoff, _IO_cookie_seekoff),
  JUMP_INIT(seekpos, _IO_default_seekpos),
  JUMP_INIT(setbuf, _IO_file_setbuf),
  JUMP_INIT(sync, _IO_file_sync),
  JUMP_INIT(doallocate, _IO_file_doallocate),
  JUMP_INIT(read, _IO_cookie_read),
  JUMP_INIT(write, _IO_cookie_write),
  JUMP_INIT(seek, _IO_cookie_seek),
  JUMP_INIT(close, _IO_cookie_close),
  JUMP_INIT(stat, _IO_default_stat),
  JUMP_INIT(showmanyc, _IO_default_showmanyc),
  JUMP_INIT(imbue, _IO_default_imbue),
};


void
_IO_cookie_init (struct _IO_cookie_file *cfile, int read_write,
		 void *cookie, _IO_cookie_io_functions_t io_functions)
{
  _IO_init (&cfile->__fp.file, 0);
  _IO_JUMPS (&cfile->__fp) = &_IO_cookie_jumps;

  cfile->__cookie = cookie;
  cfile->__io_functions = io_functions;

  _IO_file_init (&cfile->__fp);

  _IO_mask_flags (&cfile->__fp.file, read_write,
		  _IO_NO_READS+_IO_NO_WRITES+_IO_IS_APPENDING);

  /* We use a negative number different from -1 for _fileno to mark that
     this special stream is not associated with a real file, but still has
     to be treated as such.  */
  cfile->__fp.file._fileno = -2;
}


_IO_FILE *
_IO_fopencookie (cookie, mode, io_functions)
     void *cookie;
     const char *mode;
     _IO_cookie_io_functions_t io_functions;
{
  int read_write;
  struct locked_FILE
  {
    struct _IO_cookie_file cfile;
#ifdef _IO_MTSAFE_IO
    _IO_lock_t lock;
#endif
  } *new_f;

  switch (*mode++)
    {
    case 'r':
      read_write = _IO_NO_WRITES;
      break;
    case 'w':
      read_write = _IO_NO_READS;
      break;
    case 'a':
      read_write = _IO_NO_READS|_IO_IS_APPENDING;
      break;
    default:
      return NULL;
  }
  if (mode[0] == '+' || (mode[0] == 'b' && mode[1] == '+'))
    read_write &= _IO_IS_APPENDING;

  new_f = (struct locked_FILE *) malloc (sizeof (struct locked_FILE));
  if (new_f == NULL)
    return NULL;
#ifdef _IO_MTSAFE_IO
  new_f->cfile.__fp.file._lock = &new_f->lock;
#endif

  _IO_cookie_init (&new_f->cfile, read_write, cookie, io_functions);

  return (_IO_FILE *) &new_f->cfile.__fp;
}

versioned_symbol (libc, _IO_fopencookie, fopencookie, GLIBC_2_2);

#if SHLIB_COMPAT (libc, GLIBC_2_0, GLIBC_2_2)

static _IO_off64_t _IO_old_cookie_seek (_IO_FILE *fp, _IO_off64_t offset,
					int dir);
_IO_FILE * _IO_old_fopencookie (void *cookie, const char *mode,
				_IO_cookie_io_functions_t io_functions);

static _IO_off64_t
attribute_compat_text_section
_IO_old_cookie_seek (fp, offset, dir)
     _IO_FILE *fp;
     _IO_off64_t offset;
     int dir;
{
  struct _IO_cookie_file *cfile = (struct _IO_cookie_file *) fp;
  int (*seek) (_IO_FILE *, _IO_off_t, int);
  int ret;

  seek = (int (*)(_IO_FILE *, _IO_off_t, int)) cfile->__io_functions.seek;
  if (seek == NULL)
    return _IO_pos_BAD;

  ret = seek (cfile->__cookie, offset, dir);

  return (ret == -1) ? _IO_pos_BAD : ret;
}

static const struct _IO_jump_t _IO_old_cookie_jumps = {
  JUMP_INIT_DUMMY,
  JUMP_INIT(finish, _IO_file_finish),
  JUMP_INIT(overflow, _IO_file_overflow),
  JUMP_INIT(underflow, _IO_file_underflow),
  JUMP_INIT(uflow, _IO_default_uflow),
  JUMP_INIT(pbackfail, _IO_default_pbackfail),
  JUMP_INIT(xsputn, _IO_file_xsputn),
  JUMP_INIT(xsgetn, _IO_default_xsgetn),
  JUMP_INIT(seekoff, _IO_cookie_seekoff),
  JUMP_INIT(seekpos, _IO_default_seekpos),
  JUMP_INIT(setbuf, _IO_file_setbuf),
  JUMP_INIT(sync, _IO_file_sync),
  JUMP_INIT(doallocate, _IO_file_doallocate),
  JUMP_INIT(read, _IO_cookie_read),
  JUMP_INIT(write, _IO_cookie_write),
  JUMP_INIT(seek, _IO_old_cookie_seek),
  JUMP_INIT(close, _IO_cookie_close),
  JUMP_INIT(stat, _IO_default_stat),
  JUMP_INIT(showmanyc, _IO_default_showmanyc),
  JUMP_INIT(imbue, _IO_default_imbue),
};

_IO_FILE *
attribute_compat_text_section
_IO_old_fopencookie (cookie, mode, io_functions)
     void *cookie;
     const char *mode;
     _IO_cookie_io_functions_t io_functions;
{
  _IO_FILE *ret;

  ret = _IO_fopencookie (cookie, mode, io_functions);
  if (ret != NULL)
    _IO_JUMPS ((struct _IO_FILE_plus *) ret) = &_IO_old_cookie_jumps;

  return ret;
}

compat_symbol (libc, _IO_old_fopencookie, fopencookie, GLIBC_2_0);

#endif
