/* Copyright (C) 1993,95,97,98,99,2000,2001,2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Written by Ulrich Drepper <drepper@cygnus.com>.
   Based on the single byte version by Per Bothner <bothner@cygnus.com>.

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

#include <assert.h>
#include <libioP.h>
#include <wchar.h>
#include <gconv.h>
#include <stdlib.h>
#include <string.h>


#ifndef _LIBC
# define _IO_new_do_write _IO_do_write
# define _IO_new_file_attach _IO_file_attach
# define _IO_new_file_close_it _IO_file_close_it
# define _IO_new_file_finish _IO_file_finish
# define _IO_new_file_fopen _IO_file_fopen
# define _IO_new_file_init _IO_file_init
# define _IO_new_file_setbuf _IO_file_setbuf
# define _IO_new_file_sync _IO_file_sync
# define _IO_new_file_overflow _IO_file_overflow
# define _IO_new_file_seekoff _IO_file_seekoff
# define _IO_new_file_underflow _IO_file_underflow
# define _IO_new_file_write _IO_file_write
# define _IO_new_file_xsputn _IO_file_xsputn
#endif


/* Convert TO_DO wide character from DATA to FP.
   Then mark FP as having empty buffers. */
int
_IO_wdo_write (fp, data, to_do)
     _IO_FILE *fp;
     const wchar_t *data;
     _IO_size_t to_do;
{
  struct _IO_codecvt *cc = fp->_codecvt;

  if (to_do > 0)
    {
      if (fp->_IO_write_end == fp->_IO_write_ptr
	  && fp->_IO_write_end != fp->_IO_write_base)
	{
	  if (_IO_new_do_write (fp, fp->_IO_write_base,
				fp->_IO_write_ptr - fp->_IO_write_base) == EOF)
	    return EOF;
	}

      do
	{
	  enum __codecvt_result result;
	  const wchar_t *new_data;

	  /* Now convert from the internal format into the external buffer.  */
	  result = (*cc->__codecvt_do_out) (cc, &fp->_wide_data->_IO_state,
					    data, data + to_do, &new_data,
					    fp->_IO_write_ptr,
					    fp->_IO_buf_end,
					    &fp->_IO_write_ptr);

	  /* Write out what we produced so far.  */
	  if (_IO_new_do_write (fp, fp->_IO_write_base,
				fp->_IO_write_ptr - fp->_IO_write_base) == EOF)
	    /* Something went wrong.  */
	    return WEOF;

	  to_do -= new_data - data;

	  /* Next see whether we had problems during the conversion.  If yes,
	     we cannot go on.  */
	  if (result != __codecvt_ok
	      && (result != __codecvt_partial || new_data - data == 0))
	    break;

	  data = new_data;
	}
      while (to_do > 0);
    }

  _IO_wsetg (fp, fp->_wide_data->_IO_buf_base, fp->_wide_data->_IO_buf_base,
	     fp->_wide_data->_IO_buf_base);
  fp->_wide_data->_IO_write_base = fp->_wide_data->_IO_write_ptr
    = fp->_wide_data->_IO_buf_base;
  fp->_wide_data->_IO_write_end = ((fp->_flags & (_IO_LINE_BUF+_IO_UNBUFFERED))
				   ? fp->_wide_data->_IO_buf_base
				   : fp->_wide_data->_IO_buf_end);

  return to_do == 0 ? 0 : WEOF;
}
INTDEF(_IO_wdo_write)


wint_t
_IO_wfile_underflow (fp)
     _IO_FILE *fp;
{
  struct _IO_codecvt *cd;
  enum __codecvt_result status;
  _IO_ssize_t count;
  int tries;
  const char *read_ptr_copy;

  if (__builtin_expect (fp->_flags & _IO_NO_READS, 0))
    {
      fp->_flags |= _IO_ERR_SEEN;
      __set_errno (EBADF);
      return WEOF;
    }
  if (fp->_wide_data->_IO_read_ptr < fp->_wide_data->_IO_read_end)
    return *fp->_wide_data->_IO_read_ptr;

  cd = fp->_codecvt;

  /* Maybe there is something left in the external buffer.  */
  if (fp->_IO_read_ptr < fp->_IO_read_end)
    {
      /* There is more in the external.  Convert it.  */
      const char *read_stop = (const char *) fp->_IO_read_ptr;

      fp->_wide_data->_IO_last_state = fp->_wide_data->_IO_state;
      fp->_wide_data->_IO_read_base = fp->_wide_data->_IO_read_ptr =
	fp->_wide_data->_IO_buf_base;
      status = (*cd->__codecvt_do_in) (cd, &fp->_wide_data->_IO_state,
				       fp->_IO_read_ptr, fp->_IO_read_end,
				       &read_stop,
				       fp->_wide_data->_IO_read_ptr,
				       fp->_wide_data->_IO_buf_end,
				       &fp->_wide_data->_IO_read_end);

      fp->_IO_read_ptr = (char *) read_stop;

      /* If we managed to generate some text return the next character.  */
      if (fp->_wide_data->_IO_read_ptr < fp->_wide_data->_IO_read_end)
	return *fp->_wide_data->_IO_read_ptr;

      if (status == __codecvt_error)
	{
	  __set_errno (EILSEQ);
	  fp->_flags |= _IO_ERR_SEEN;
	  return WEOF;
	}

      /* Move the remaining content of the read buffer to the beginning.  */
      memmove (fp->_IO_buf_base, fp->_IO_read_ptr,
	       fp->_IO_read_end - fp->_IO_read_ptr);
      fp->_IO_read_end = (fp->_IO_buf_base
			  + (fp->_IO_read_end - fp->_IO_read_ptr));
      fp->_IO_read_base = fp->_IO_read_ptr = fp->_IO_buf_base;
    }
  else
    fp->_IO_read_base = fp->_IO_read_ptr = fp->_IO_read_end =
      fp->_IO_buf_base;

  if (fp->_IO_buf_base == NULL)
    {
      /* Maybe we already have a push back pointer.  */
      if (fp->_IO_save_base != NULL)
	{
	  free (fp->_IO_save_base);
	  fp->_flags &= ~_IO_IN_BACKUP;
	}
      INTUSE(_IO_doallocbuf) (fp);

      fp->_IO_read_base = fp->_IO_read_ptr = fp->_IO_read_end =
	fp->_IO_buf_base;
    }

  fp->_IO_write_base = fp->_IO_write_ptr = fp->_IO_write_end =
    fp->_IO_buf_base;

  if (fp->_wide_data->_IO_buf_base == NULL)
    {
      /* Maybe we already have a push back pointer.  */
      if (fp->_wide_data->_IO_save_base != NULL)
	{
	  free (fp->_wide_data->_IO_save_base);
	  fp->_flags &= ~_IO_IN_BACKUP;
	}
      INTUSE(_IO_wdoallocbuf) (fp);
    }

  /* Flush all line buffered files before reading. */
  /* FIXME This can/should be moved to genops ?? */
  if (fp->_flags & (_IO_LINE_BUF|_IO_UNBUFFERED))
    {
#if 0
      INTUSE(_IO_flush_all_linebuffered) ();
#else
      /* We used to flush all line-buffered stream.  This really isn't
	 required by any standard.  My recollection is that
	 traditional Unix systems did this for stdout.  stderr better
	 not be line buffered.  So we do just that here
	 explicitly.  --drepper */
      _IO_cleanup_region_start ((void (*) __P ((void *))) _IO_funlockfile,
				_IO_stdout);
      _IO_flockfile (_IO_stdout);

      if ((_IO_stdout->_flags & (_IO_LINKED | _IO_NO_WRITES | _IO_LINE_BUF))
	  == (_IO_LINKED | _IO_LINE_BUF))
	_IO_OVERFLOW (_IO_stdout, EOF);

      _IO_funlockfile (_IO_stdout);
      _IO_cleanup_region_end (0);
#endif
    }

  INTUSE(_IO_switch_to_get_mode) (fp);

  fp->_wide_data->_IO_read_base = fp->_wide_data->_IO_read_ptr =
    fp->_wide_data->_IO_buf_base;
  fp->_wide_data->_IO_read_end = fp->_wide_data->_IO_buf_base;
  fp->_wide_data->_IO_write_base = fp->_wide_data->_IO_write_ptr =
    fp->_wide_data->_IO_write_end = fp->_wide_data->_IO_buf_base;

  tries = 0;
 again:
  count = _IO_SYSREAD (fp, fp->_IO_read_end,
		       fp->_IO_buf_end - fp->_IO_read_end);
  if (count <= 0)
    {
      if (count == 0 && tries == 0)
	fp->_flags |= _IO_EOF_SEEN;
      else
	fp->_flags |= _IO_ERR_SEEN, count = 0;
    }
  fp->_IO_read_end += count;
  if (count == 0)
    {
      if (tries != 0)
	/* There are some bytes in the external buffer but they don't
           convert to anything.  */
	__set_errno (EILSEQ);
      return WEOF;
    }
  if (fp->_offset != _IO_pos_BAD)
    _IO_pos_adjust (fp->_offset, count);

  /* Now convert the read input.  */
  fp->_wide_data->_IO_last_state = fp->_wide_data->_IO_state;
  fp->_IO_read_base = fp->_IO_read_ptr;
  status = (*cd->__codecvt_do_in) (cd, &fp->_wide_data->_IO_state,
				   fp->_IO_read_ptr, fp->_IO_read_end,
				   &read_ptr_copy,
				   fp->_wide_data->_IO_read_end,
				   fp->_wide_data->_IO_buf_end,
				   &fp->_wide_data->_IO_read_end);

  fp->_IO_read_ptr = (char *) read_ptr_copy;
  if (fp->_wide_data->_IO_read_end == fp->_wide_data->_IO_buf_base)
    {
      if (status == __codecvt_error || fp->_IO_read_end == fp->_IO_buf_end)
	{
	  __set_errno (EILSEQ);
	  fp->_flags |= _IO_ERR_SEEN;
	  return WEOF;
	}

      /* The read bytes make no complete character.  Try reading again.  */
      assert (status == __codecvt_partial);
      ++tries;
      goto again;
    }

  return *fp->_wide_data->_IO_read_ptr;
}
INTDEF(_IO_wfile_underflow)


#ifdef HAVE_MMAP
static wint_t
_IO_wfile_underflow_mmap (_IO_FILE *fp)
{
  struct _IO_codecvt *cd;
  enum __codecvt_result status;
  const char *read_stop;

  if (__builtin_expect (fp->_flags & _IO_NO_READS, 0))
    {
      fp->_flags |= _IO_ERR_SEEN;
      __set_errno (EBADF);
      return WEOF;
    }
  if (fp->_wide_data->_IO_read_ptr < fp->_wide_data->_IO_read_end)
    return *fp->_wide_data->_IO_read_ptr;

  cd = fp->_codecvt;

  /* Maybe there is something left in the external buffer.  */
  if (fp->_IO_read_ptr >= fp->_IO_read_end
      /* No.  But maybe the read buffer is not fully set up.  */
      && _IO_file_underflow_mmap (fp) == EOF)
    /* Nothing available.  _IO_file_underflow_mmap has set the EOF or error
       flags as appropriate.  */
    return WEOF;

  /* There is more in the external.  Convert it.  */
  read_stop = (const char *) fp->_IO_read_ptr;

  if (fp->_wide_data->_IO_buf_base == NULL)
    {
      /* Maybe we already have a push back pointer.  */
      if (fp->_wide_data->_IO_save_base != NULL)
	{
	  free (fp->_wide_data->_IO_save_base);
	  fp->_flags &= ~_IO_IN_BACKUP;
	}
      INTUSE(_IO_wdoallocbuf) (fp);
    }

  fp->_wide_data->_IO_last_state = fp->_wide_data->_IO_state;
  fp->_wide_data->_IO_read_base = fp->_wide_data->_IO_read_ptr =
    fp->_wide_data->_IO_buf_base;
  status = (*cd->__codecvt_do_in) (cd, &fp->_wide_data->_IO_state,
				   fp->_IO_read_ptr, fp->_IO_read_end,
				   &read_stop,
				   fp->_wide_data->_IO_read_ptr,
				   fp->_wide_data->_IO_buf_end,
				   &fp->_wide_data->_IO_read_end);

  fp->_IO_read_ptr = (char *) read_stop;

  /* If we managed to generate some text return the next character.  */
  if (fp->_wide_data->_IO_read_ptr < fp->_wide_data->_IO_read_end)
    return *fp->_wide_data->_IO_read_ptr;

  /* There is some garbage at the end of the file.  */
  __set_errno (EILSEQ);
  fp->_flags |= _IO_ERR_SEEN;
  return WEOF;
}

static wint_t
_IO_wfile_underflow_maybe_mmap (_IO_FILE *fp)
{
  /* This is the first read attempt.  Doing the underflow will choose mmap
     or vanilla operations and then punt to the chosen underflow routine.
     Then we can punt to ours.  */
  if (_IO_file_underflow_maybe_mmap (fp) == EOF)
    return WEOF;

  return _IO_WUNDERFLOW (fp);
}
#endif


wint_t
_IO_wfile_overflow (f, wch)
     _IO_FILE *f;
     wint_t wch;
{
  if (f->_flags & _IO_NO_WRITES) /* SET ERROR */
    {
      f->_flags |= _IO_ERR_SEEN;
      __set_errno (EBADF);
      return WEOF;
    }
  /* If currently reading or no buffer allocated. */
  if ((f->_flags & _IO_CURRENTLY_PUTTING) == 0)
    {
      /* Allocate a buffer if needed. */
      if (f->_wide_data->_IO_write_base == 0)
	{
	  INTUSE(_IO_wdoallocbuf) (f);
	  _IO_wsetg (f, f->_wide_data->_IO_buf_base,
		     f->_wide_data->_IO_buf_base, f->_wide_data->_IO_buf_base);

	  if (f->_IO_write_base == NULL)
	    {
	      INTUSE(_IO_doallocbuf) (f);
	      _IO_setg (f, f->_IO_buf_base, f->_IO_buf_base, f->_IO_buf_base);
	    }
	}
      else
	{
	  /* Otherwise must be currently reading.  If _IO_read_ptr
	     (and hence also _IO_read_end) is at the buffer end,
	     logically slide the buffer forwards one block (by setting
	     the read pointers to all point at the beginning of the
	     block).  This makes room for subsequent output.
	     Otherwise, set the read pointers to _IO_read_end (leaving
	     that alone, so it can continue to correspond to the
	     external position). */
	  if (f->_wide_data->_IO_read_ptr == f->_wide_data->_IO_buf_end)
	    {
	      f->_IO_read_end = f->_IO_read_ptr = f->_IO_buf_base;
	      f->_wide_data->_IO_read_end = f->_wide_data->_IO_read_ptr =
		f->_wide_data->_IO_buf_base;
	    }
	}
      f->_wide_data->_IO_write_ptr = f->_wide_data->_IO_read_ptr;
      f->_wide_data->_IO_write_base = f->_wide_data->_IO_write_ptr;
      f->_wide_data->_IO_write_end = f->_wide_data->_IO_buf_end;
      f->_wide_data->_IO_read_base = f->_wide_data->_IO_read_ptr =
	f->_wide_data->_IO_read_end;

      f->_IO_write_ptr = f->_IO_read_ptr;
      f->_IO_write_base = f->_IO_write_ptr;
      f->_IO_write_end = f->_IO_buf_end;
      f->_IO_read_base = f->_IO_read_ptr = f->_IO_read_end;

      f->_flags |= _IO_CURRENTLY_PUTTING;
      if (f->_flags & (_IO_LINE_BUF+_IO_UNBUFFERED))
	f->_wide_data->_IO_write_end = f->_wide_data->_IO_write_ptr;
    }
  if (wch == WEOF)
    return _IO_do_flush (f);
  if (f->_wide_data->_IO_write_ptr == f->_wide_data->_IO_buf_end)
    /* Buffer is really full */
    if (_IO_do_flush (f) == EOF)
      return WEOF;
  *f->_wide_data->_IO_write_ptr++ = wch;
  if ((f->_flags & _IO_UNBUFFERED)
      || ((f->_flags & _IO_LINE_BUF) && wch == L'\n'))
    if (_IO_do_flush (f) == EOF)
      return WEOF;
  return wch;
}
INTDEF(_IO_wfile_overflow)

wint_t
_IO_wfile_sync (fp)
     _IO_FILE *fp;
{
  _IO_ssize_t delta;
  wint_t retval = 0;

  /*    char* ptr = cur_ptr(); */
  if (fp->_wide_data->_IO_write_ptr > fp->_wide_data->_IO_write_base)
    if (_IO_do_flush (fp))
      return WEOF;
  delta = fp->_wide_data->_IO_read_ptr - fp->_wide_data->_IO_read_end;
  if (delta != 0)
    {
      /* We have to find out how many bytes we have to go back in the
	 external buffer.  */
      struct _IO_codecvt *cv = fp->_codecvt;
      _IO_off64_t new_pos;

      int clen = (*cv->__codecvt_do_encoding) (cv);

      if (clen > 0)
	/* It is easy, a fixed number of input bytes are used for each
	   wide character.  */
	delta *= clen;
      else
	{
	  /* We have to find out the hard way how much to back off.
             To do this we determine how much input we needed to
             generate the wide characters up to the current reading
             position.  */
	  int nread;

	  fp->_wide_data->_IO_state = fp->_wide_data->_IO_last_state;
	  nread = (*cv->__codecvt_do_length) (cv, &fp->_wide_data->_IO_state,
					      fp->_IO_read_base,
					      fp->_IO_read_end, delta);
	  fp->_IO_read_ptr = fp->_IO_read_base + nread;
	  delta = -(fp->_IO_read_end - fp->_IO_read_base - nread);
	}

      new_pos = _IO_SYSSEEK (fp, delta, 1);
      if (new_pos != (_IO_off64_t) EOF)
	{
	  fp->_wide_data->_IO_read_end = fp->_wide_data->_IO_read_ptr;
	  fp->_IO_read_end = fp->_IO_read_ptr;
	}
#ifdef ESPIPE
      else if (errno == ESPIPE)
	; /* Ignore error from unseekable devices. */
#endif
      else
	retval = WEOF;
    }
  if (retval != WEOF)
    fp->_offset = _IO_pos_BAD;
  /* FIXME: Cleanup - can this be shared? */
  /*    setg(base(), ptr, ptr); */
  return retval;
}
INTDEF(_IO_wfile_sync)

_IO_off64_t
_IO_wfile_seekoff (fp, offset, dir, mode)
     _IO_FILE *fp;
     _IO_off64_t offset;
     int dir;
     int mode;
{
  _IO_off64_t result;
  _IO_off64_t delta, new_offset;
  long int count;
  /* POSIX.1 8.2.3.7 says that after a call the fflush() the file
     offset of the underlying file must be exact.  */
  int must_be_exact = ((fp->_wide_data->_IO_read_base
			== fp->_wide_data->_IO_read_end)
		       && (fp->_wide_data->_IO_write_base
			   == fp->_wide_data->_IO_write_ptr));

  if (mode == 0)
    {
      /* XXX For wide stream with backup store it is not very
	 reasonable to determine the offset.  The pushed-back
	 character might require a state change and we need not be
	 able to compute the initial state by reverse transformation
	 since there is no guarantee of symmetry.  So we don't even
	 try and return an error.  */
      if (_IO_in_backup (fp))
	{
	  if (fp->_wide_data->_IO_read_ptr < fp->_wide_data->_IO_read_end)
	    {
	      __set_errno (EINVAL);
	      return -1;
	    }

	  /* There is no more data in the backup buffer.  We can
	     switch back.  */
	  INTUSE(_IO_switch_to_main_wget_area) (fp);
	}

      dir = _IO_seek_cur, offset = 0; /* Don't move any pointers. */
    }

  /* Flush unwritten characters.
     (This may do an unneeded write if we seek within the buffer.
     But to be able to switch to reading, we would need to set
     egptr to ptr.  That can't be done in the current design,
     which assumes file_ptr() is eGptr.  Anyway, since we probably
     end up flushing when we close(), it doesn't make much difference.)
     FIXME: simulate mem-mapped files. */

  if (fp->_wide_data->_IO_write_ptr > fp->_wide_data->_IO_write_base
      || _IO_in_put_mode (fp))
    if (INTUSE(_IO_switch_to_wget_mode) (fp))
      return WEOF;

  if (fp->_wide_data->_IO_buf_base == NULL)
    {
      /* It could be that we already have a pushback buffer.  */
      if (fp->_wide_data->_IO_read_base != NULL)
	{
	  free (fp->_wide_data->_IO_read_base);
	  fp->_flags &= ~_IO_IN_BACKUP;
	}
      INTUSE(_IO_doallocbuf) (fp);
      _IO_setp (fp, fp->_IO_buf_base, fp->_IO_buf_base);
      _IO_setg (fp, fp->_IO_buf_base, fp->_IO_buf_base, fp->_IO_buf_base);
      _IO_wsetp (fp, fp->_wide_data->_IO_buf_base,
		 fp->_wide_data->_IO_buf_base);
      _IO_wsetg (fp, fp->_wide_data->_IO_buf_base,
		 fp->_wide_data->_IO_buf_base, fp->_wide_data->_IO_buf_base);
    }

  switch (dir)
    {
      struct _IO_codecvt *cv;
      int clen;

    case _IO_seek_cur:
      /* Adjust for read-ahead (bytes is buffer).  To do this we must
         find out which position in the external buffer corresponds to
         the current position in the internal buffer.  */
      cv = fp->_codecvt;
      clen = (*cv->__codecvt_do_encoding) (cv);

      if (clen > 0)
	offset -= (fp->_wide_data->_IO_read_end
		   - fp->_wide_data->_IO_read_ptr) * clen;
      else
	{
	  int nread;

	  delta = fp->_wide_data->_IO_read_ptr - fp->_wide_data->_IO_read_base;
	  fp->_wide_data->_IO_state = fp->_wide_data->_IO_last_state;
	  nread = (*cv->__codecvt_do_length) (cv, &fp->_wide_data->_IO_state,
					      fp->_IO_read_base,
					      fp->_IO_read_end, delta);
	  fp->_IO_read_ptr = fp->_IO_read_base + nread;
	  fp->_wide_data->_IO_read_end = fp->_wide_data->_IO_read_ptr;
	  offset -= fp->_IO_read_end - fp->_IO_read_base - nread;
	}

      if (fp->_offset == _IO_pos_BAD)
	goto dumb;
      /* Make offset absolute, assuming current pointer is file_ptr(). */
      offset += fp->_offset;

      dir = _IO_seek_set;
      break;
    case _IO_seek_set:
      break;
    case _IO_seek_end:
      {
	struct _G_stat64 st;
	if (_IO_SYSSTAT (fp, &st) == 0 && S_ISREG (st.st_mode))
	  {
	    offset += st.st_size;
	    dir = _IO_seek_set;
	  }
	else
	  goto dumb;
      }
    }
  /* At this point, dir==_IO_seek_set. */

  /* If we are only interested in the current position we've found it now.  */
  if (mode == 0)
    return offset;

  /* If destination is within current buffer, optimize: */
  if (fp->_offset != _IO_pos_BAD && fp->_IO_read_base != NULL
      && !_IO_in_backup (fp))
    {
      /* Offset relative to start of main get area. */
      _IO_off64_t rel_offset = (offset - fp->_offset
				+ (fp->_IO_read_end - fp->_IO_buf_base));
      if (rel_offset >= 0)
	{
#if 0
	  if (_IO_in_backup (fp))
	    _IO_switch_to_main_get_area (fp);
#endif
	  if (rel_offset <= fp->_IO_read_end - fp->_IO_buf_base)
	    {
	      enum __codecvt_result status;
	      struct _IO_codecvt *cd = fp->_codecvt;
	      const char *read_ptr_copy;

	      fp->_IO_read_ptr = fp->_IO_read_base + rel_offset;
	      _IO_setp (fp, fp->_IO_buf_base, fp->_IO_buf_base);

	      /* Now set the pointer for the internal buffer.  This
                 might be an iterative process.  Though the read
                 pointer is somewhere in the current external buffer
                 this does not mean we can convert this whole buffer
                 at once fitting in the internal buffer.  */
	      fp->_wide_data->_IO_state = fp->_wide_data->_IO_last_state;
	      read_ptr_copy = fp->_IO_read_base;
	      fp->_wide_data->_IO_read_ptr = fp->_wide_data->_IO_read_base;
	      do
		{
		  wchar_t buffer[1024];
		  wchar_t *ignore;
		  status = (*cd->__codecvt_do_in) (cd,
						   &fp->_wide_data->_IO_state,
						   read_ptr_copy,
						   fp->_IO_read_ptr,
						   &read_ptr_copy,
						   buffer,
						   buffer
						   + (sizeof (buffer)
						      / sizeof (buffer[0])),
						   &ignore);
		  if (status != __codecvt_ok && status != __codecvt_partial)
		    {
		      fp->_flags |= _IO_ERR_SEEN;
		      goto dumb;
		    }
		}
	      while (read_ptr_copy != fp->_IO_read_ptr);

	      fp->_wide_data->_IO_read_ptr = fp->_wide_data->_IO_read_base;

	      _IO_mask_flags (fp, 0, _IO_EOF_SEEN);
	      goto resync;
	    }
#ifdef TODO
	    /* If we have streammarkers, seek forward by reading ahead. */
	    if (_IO_have_markers (fp))
	      {
		int to_skip = rel_offset
		  - (fp->_IO_read_ptr - fp->_IO_read_base);
		if (ignore (to_skip) != to_skip)
		  goto dumb;
		_IO_mask_flags (fp, 0, _IO_EOF_SEEN);
		goto resync;
	      }
#endif
	}
#ifdef TODO
      if (rel_offset < 0 && rel_offset >= Bbase () - Bptr ())
	{
	  if (!_IO_in_backup (fp))
	    _IO_switch_to_backup_area (fp);
	  gbump (fp->_IO_read_end + rel_offset - fp->_IO_read_ptr);
	  _IO_mask_flags (fp, 0, _IO_EOF_SEEN);
	  goto resync;
	}
#endif
    }

#ifdef TODO
  INTUSE(_IO_unsave_markers) (fp);
#endif

  if (fp->_flags & _IO_NO_READS)
    goto dumb;

  /* Try to seek to a block boundary, to improve kernel page management. */
  new_offset = offset & ~(fp->_IO_buf_end - fp->_IO_buf_base - 1);
  delta = offset - new_offset;
  if (delta > fp->_IO_buf_end - fp->_IO_buf_base)
    {
      new_offset = offset;
      delta = 0;
    }
  result = _IO_SYSSEEK (fp, new_offset, 0);
  if (result < 0)
    return EOF;
  if (delta == 0)
    count = 0;
  else
    {
      count = _IO_SYSREAD (fp, fp->_IO_buf_base,
			   (must_be_exact
			    ? delta : fp->_IO_buf_end - fp->_IO_buf_base));
      if (count < delta)
	{
	  /* We weren't allowed to read, but try to seek the remainder. */
	  offset = count == EOF ? delta : delta-count;
	  dir = _IO_seek_cur;
	  goto dumb;
	}
    }
  _IO_setg (fp, fp->_IO_buf_base, fp->_IO_buf_base + delta,
	    fp->_IO_buf_base + count);
  _IO_setp (fp, fp->_IO_buf_base, fp->_IO_buf_base);
  fp->_offset = result + count;
  _IO_mask_flags (fp, 0, _IO_EOF_SEEN);
  return offset;
 dumb:

  INTUSE(_IO_unsave_markers) (fp);
  result = _IO_SYSSEEK (fp, offset, dir);
  if (result != EOF)
    {
      _IO_mask_flags (fp, 0, _IO_EOF_SEEN);
      fp->_offset = result;
      _IO_setg (fp, fp->_IO_buf_base, fp->_IO_buf_base, fp->_IO_buf_base);
      _IO_setp (fp, fp->_IO_buf_base, fp->_IO_buf_base);
      _IO_wsetg (fp, fp->_wide_data->_IO_buf_base,
		 fp->_wide_data->_IO_buf_base, fp->_wide_data->_IO_buf_base);
      _IO_wsetp (fp, fp->_wide_data->_IO_buf_base,
		 fp->_wide_data->_IO_buf_base);
    }
  return result;

resync:
  /* We need to do it since it is possible that the file offset in
     the kernel may be changed behind our back. It may happen when
     we fopen a file and then do a fork. One process may access the
     the file and the kernel file offset will be changed. */
  if (fp->_offset >= 0)
    _IO_SYSSEEK (fp, fp->_offset, 0);

  return offset;
}
INTDEF(_IO_wfile_seekoff)


_IO_size_t
_IO_wfile_xsputn (f, data, n)
     _IO_FILE *f;
     const void *data;
     _IO_size_t n;
{
  register const wchar_t *s = (const wchar_t *) data;
  _IO_size_t to_do = n;
  int must_flush = 0;
  _IO_size_t count;

  if (n <= 0)
    return 0;
  /* This is an optimized implementation.
     If the amount to be written straddles a block boundary
     (or the filebuf is unbuffered), use sys_write directly. */

  /* First figure out how much space is available in the buffer. */
  count = f->_wide_data->_IO_write_end - f->_wide_data->_IO_write_ptr;
  if ((f->_flags & _IO_LINE_BUF) && (f->_flags & _IO_CURRENTLY_PUTTING))
    {
      count = f->_wide_data->_IO_buf_end - f->_wide_data->_IO_write_ptr;
      if (count >= n)
	{
	  register const wchar_t *p;
	  for (p = s + n; p > s; )
	    {
	      if (*--p == L'\n')
		{
		  count = p - s + 1;
		  must_flush = 1;
		  break;
		}
	    }
	}
    }
  /* Then fill the buffer. */
  if (count > 0)
    {
      if (count > to_do)
	count = to_do;
      if (count > 20)
	{
#ifdef _LIBC
	  f->_wide_data->_IO_write_ptr =
	    __wmempcpy (f->_wide_data->_IO_write_ptr, s, count);
#else
	  wmemcpy (f->_wide_data->_IO_write_ptr, s, count);
	  f->_wide_data->_IO_write_ptr += count;
#endif
	  s += count;
	}
      else
	{
	  register wchar_t *p = f->_wide_data->_IO_write_ptr;
	  register int i = (int) count;
	  while (--i >= 0)
	    *p++ = *s++;
	  f->_wide_data->_IO_write_ptr = p;
	}
      to_do -= count;
    }
  if (to_do > 0)
    to_do -= INTUSE(_IO_wdefault_xsputn) (f, s, to_do);
  if (must_flush
      && f->_wide_data->_IO_write_ptr != f->_wide_data->_IO_write_base)
    INTUSE(_IO_wdo_write) (f, f->_wide_data->_IO_write_base,
			   f->_wide_data->_IO_write_ptr
			   - f->_wide_data->_IO_write_base);

  return n - to_do;
}
INTDEF(_IO_wfile_xsputn)


struct _IO_jump_t _IO_wfile_jumps =
{
  JUMP_INIT_DUMMY,
  JUMP_INIT(finish, _IO_new_file_finish),
  JUMP_INIT(overflow, (_IO_overflow_t) INTUSE(_IO_wfile_overflow)),
  JUMP_INIT(underflow, (_IO_underflow_t) INTUSE(_IO_wfile_underflow)),
  JUMP_INIT(uflow, (_IO_underflow_t) INTUSE(_IO_wdefault_uflow)),
  JUMP_INIT(pbackfail, (_IO_pbackfail_t) INTUSE(_IO_wdefault_pbackfail)),
  JUMP_INIT(xsputn, INTUSE(_IO_wfile_xsputn)),
  JUMP_INIT(xsgetn, INTUSE(_IO_file_xsgetn)),
  JUMP_INIT(seekoff, INTUSE(_IO_wfile_seekoff)),
  JUMP_INIT(seekpos, _IO_default_seekpos),
  JUMP_INIT(setbuf, _IO_new_file_setbuf),
  JUMP_INIT(sync, (_IO_sync_t) INTUSE(_IO_wfile_sync)),
  JUMP_INIT(doallocate, _IO_wfile_doallocate),
  JUMP_INIT(read, INTUSE(_IO_file_read)),
  JUMP_INIT(write, _IO_new_file_write),
  JUMP_INIT(seek, INTUSE(_IO_file_seek)),
  JUMP_INIT(close, INTUSE(_IO_file_close)),
  JUMP_INIT(stat, INTUSE(_IO_file_stat)),
  JUMP_INIT(showmanyc, _IO_default_showmanyc),
  JUMP_INIT(imbue, _IO_default_imbue)
};
INTVARDEF(_IO_wfile_jumps)


#ifdef HAVE_MMAP
struct _IO_jump_t _IO_wfile_jumps_mmap =
{
  JUMP_INIT_DUMMY,
  JUMP_INIT(finish, _IO_new_file_finish),
  JUMP_INIT(overflow, (_IO_overflow_t) INTUSE(_IO_wfile_overflow)),
  JUMP_INIT(underflow, (_IO_underflow_t) _IO_wfile_underflow_mmap),
  JUMP_INIT(uflow, (_IO_underflow_t) INTUSE(_IO_wdefault_uflow)),
  JUMP_INIT(pbackfail, (_IO_pbackfail_t) INTUSE(_IO_wdefault_pbackfail)),
  JUMP_INIT(xsputn, INTUSE(_IO_wfile_xsputn)),
  JUMP_INIT(xsgetn, INTUSE(_IO_file_xsgetn)),
  JUMP_INIT(seekoff, INTUSE(_IO_wfile_seekoff)),
  JUMP_INIT(seekpos, _IO_default_seekpos),
  JUMP_INIT(setbuf, _IO_file_setbuf_mmap),
  JUMP_INIT(sync, (_IO_sync_t) INTUSE(_IO_wfile_sync)),
  JUMP_INIT(doallocate, _IO_wfile_doallocate),
  JUMP_INIT(read, INTUSE(_IO_file_read)),
  JUMP_INIT(write, _IO_new_file_write),
  JUMP_INIT(seek, INTUSE(_IO_file_seek)),
  JUMP_INIT(close, _IO_file_close_mmap),
  JUMP_INIT(stat, INTUSE(_IO_file_stat)),
  JUMP_INIT(showmanyc, _IO_default_showmanyc),
  JUMP_INIT(imbue, _IO_default_imbue)
};

struct _IO_jump_t _IO_wfile_jumps_maybe_mmap =
{
  JUMP_INIT_DUMMY,
  JUMP_INIT(finish, _IO_new_file_finish),
  JUMP_INIT(overflow, (_IO_overflow_t) INTUSE(_IO_wfile_overflow)),
  JUMP_INIT(underflow, (_IO_underflow_t) _IO_wfile_underflow_maybe_mmap),
  JUMP_INIT(uflow, (_IO_underflow_t) INTUSE(_IO_wdefault_uflow)),
  JUMP_INIT(pbackfail, (_IO_pbackfail_t) INTUSE(_IO_wdefault_pbackfail)),
  JUMP_INIT(xsputn, INTUSE(_IO_wfile_xsputn)),
  JUMP_INIT(xsgetn, INTUSE(_IO_file_xsgetn)),
  JUMP_INIT(seekoff, INTUSE(_IO_wfile_seekoff)),
  JUMP_INIT(seekpos, _IO_default_seekpos),
  JUMP_INIT(setbuf, _IO_file_setbuf_mmap),
  JUMP_INIT(sync, (_IO_sync_t) INTUSE(_IO_wfile_sync)),
  JUMP_INIT(doallocate, _IO_wfile_doallocate),
  JUMP_INIT(read, INTUSE(_IO_file_read)),
  JUMP_INIT(write, _IO_new_file_write),
  JUMP_INIT(seek, INTUSE(_IO_file_seek)),
  JUMP_INIT(close, INTUSE(_IO_file_close)),
  JUMP_INIT(stat, INTUSE(_IO_file_stat)),
  JUMP_INIT(showmanyc, _IO_default_showmanyc),
  JUMP_INIT(imbue, _IO_default_imbue)
};
#endif
