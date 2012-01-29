/* Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
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
#ifdef _LIBC
# include <dlfcn.h>
# include <wchar.h>
#endif
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef _LIBC
# include <langinfo.h>
# include <locale/localeinfo.h>
# include <wcsmbs/wcsmbsload.h>
# include <iconv/gconv_int.h>
# include <shlib-compat.h>
#endif

/* Prototypes of libio's codecvt functions.  */
static enum __codecvt_result do_out (struct _IO_codecvt *codecvt,
				     __mbstate_t *statep,
				     const wchar_t *from_start,
				     const wchar_t *from_end,
				     const wchar_t **from_stop, char *to_start,
				     char *to_end, char **to_stop);
static enum __codecvt_result do_unshift (struct _IO_codecvt *codecvt,
					 __mbstate_t *statep, char *to_start,
					 char *to_end, char **to_stop);
static enum __codecvt_result do_in (struct _IO_codecvt *codecvt,
				    __mbstate_t *statep,
				    const char *from_start,
				    const char *from_end,
				    const char **from_stop, wchar_t *to_start,
				    wchar_t *to_end, wchar_t **to_stop);
static int do_encoding (struct _IO_codecvt *codecvt);
static int do_length (struct _IO_codecvt *codecvt, __mbstate_t *statep,
		      const char *from_start,
		      const char *from_end, _IO_size_t max);
static int do_max_length (struct _IO_codecvt *codecvt);
static int do_always_noconv (struct _IO_codecvt *codecvt);


/* The functions used in `codecvt' for libio are always the same.  */
struct _IO_codecvt __libio_codecvt =
{
  .__codecvt_destr = NULL,		/* Destructor, never used.  */
  .__codecvt_do_out = do_out,
  .__codecvt_do_unshift = do_unshift,
  .__codecvt_do_in = do_in,
  .__codecvt_do_encoding = do_encoding,
  .__codecvt_do_always_noconv = do_always_noconv,
  .__codecvt_do_length = do_length,
  .__codecvt_do_max_length = do_max_length
};


#ifdef _LIBC
struct __gconv_trans_data __libio_translit attribute_hidden =
{
  .__trans_fct = NULL
};
#endif

/* Return orientation of stream.  If mode is nonzero try to change
 * the orientation first.
 */

#undef _IO_fwide

int
_IO_fwide(_IO_FILE *fp, int mode)
{
	/* Normalize the value.  */
	mode = mode < 0 ? -1 : (mode == 0 ? 0 : 1);

	if (mode == 0) {
		/* The caller simply wants to know about the current orientation. */
		return fp->_mode;
	}

#if defined SHARED && defined _LIBC \
    && SHLIB_COMPAT (libc, GLIBC_2_0, GLIBC_2_1)
  if (__builtin_expect (&_IO_stdin_used == NULL, 0)
      && (fp == _IO_stdin ||  fp == _IO_stdout || fp == _IO_stderr))
    /* This is for a stream in the glibc 2.0 format.  */
    return -1;
#endif

	if (fp->_mode != 0) {
		/* The orientation already has been determined.  */
		return fp->_mode;
	}

	{
		struct _IO_codecvt *cc = fp->_codecvt = &fp->_wide_data->_codecvt;

		fp->_wide_data->_IO_read_ptr = fp->_wide_data->_IO_read_end;
		fp->_wide_data->_IO_write_ptr = fp->_wide_data->_IO_write_base;

		/* Get the character conversion functions based on the currently
		 * selected locale for LC_CTYPE.
		 */
#ifdef _LIBC
      {
	struct gconv_fcts fcts;

	/* Clear the state.  We start all over again.  */
	memset (&fp->_wide_data->_IO_state, '\0', sizeof (__mbstate_t));
	memset (&fp->_wide_data->_IO_last_state, '\0', sizeof (__mbstate_t));

	__wcsmbs_clone_conv (&fcts);
	assert (fcts.towc_nsteps == 1);
	assert (fcts.tomb_nsteps == 1);

	/* The functions are always the same.  */
	*cc = __libio_codecvt;

	cc->__cd_in.__cd.__nsteps = fcts.towc_nsteps;
	cc->__cd_in.__cd.__steps = fcts.towc;

	cc->__cd_in.__cd.__data[0].__invocation_counter = 0;
	cc->__cd_in.__cd.__data[0].__internal_use = 1;
	cc->__cd_in.__cd.__data[0].__flags = __GCONV_IS_LAST;
	cc->__cd_in.__cd.__data[0].__statep = &fp->_wide_data->_IO_state;

	/* XXX For now no transliteration.  */
	cc->__cd_in.__cd.__data[0].__trans = NULL;

	cc->__cd_out.__cd.__nsteps = fcts.tomb_nsteps;
	cc->__cd_out.__cd.__steps = fcts.tomb;

	cc->__cd_out.__cd.__data[0].__invocation_counter = 0;
	cc->__cd_out.__cd.__data[0].__internal_use = 1;
	cc->__cd_out.__cd.__data[0].__flags = __GCONV_IS_LAST;
	cc->__cd_out.__cd.__data[0].__statep = &fp->_wide_data->_IO_state;

	/* And now the transliteration.  */
	cc->__cd_out.__cd.__data[0].__trans = &__libio_translit;
      }
#else
# ifdef _GLIBCPP_USE_WCHAR_T
      {
	/* Determine internal and external character sets.

	   XXX For now we make our life easy: we assume a fixed internal
	   encoding (as most sane systems have; hi HP/UX!).  If somebody
	   cares about systems which changing internal charsets they
	   should come up with a solution for the determination of the
	   currently used internal character set.  */
	const char *internal_ccs = _G_INTERNAL_CCS;
	const char *external_ccs = NULL;

#  ifdef HAVE_NL_LANGINFO
	external_ccs = nl_langinfo (CODESET);
#  endif
	if (external_ccs == NULL)
	  external_ccs = "ISO-8859-1";

	cc->__cd_in = iconv_open (internal_ccs, external_ccs);
	if (cc->__cd_in != (iconv_t) -1)
	  cc->__cd_out = iconv_open (external_ccs, internal_ccs);

	if (cc->__cd_in == (iconv_t) -1 || cc->__cd_out == (iconv_t) -1)
	  {
	    if (cc->__cd_in != (iconv_t) -1)
	      iconv_close (cc->__cd_in);
	    /* XXX */
	    abort ();
	  }
      }
# else
#  error "somehow determine this from LC_CTYPE"
# endif
#endif

      /* From now on use the wide character callback functions.  */
      ((struct _IO_FILE_plus *) fp)->vtable = fp->_wide_data->_wide_vtable;

      /* One last twist: we get the current stream position.  The wide
	 char streams have much more problems with not knowing the
	 current position and so we should disable the optimization
	 which allows the functions without knowing the position.  */
      fp->_offset = _IO_SYSSEEK (fp, 0, _IO_seek_cur);
    }
	/* Set the mode now.  */
	fp->_mode = mode;

	return mode;
}

static enum __codecvt_result
do_out (struct _IO_codecvt *codecvt, __mbstate_t *statep,
	const wchar_t *from_start, const wchar_t *from_end,
	const wchar_t **from_stop, char *to_start, char *to_end,
	char **to_stop)
{
  enum __codecvt_result result;

#ifdef _LIBC
  struct __gconv_step *gs = codecvt->__cd_out.__cd.__steps;
  int status;
  size_t dummy;
  const unsigned char *from_start_copy = (unsigned char *) from_start;

  codecvt->__cd_out.__cd.__data[0].__outbuf = to_start;
  codecvt->__cd_out.__cd.__data[0].__outbufend = to_end;
  codecvt->__cd_out.__cd.__data[0].__statep = statep;

  status = DL_CALL_FCT (gs->__fct,
			(gs, codecvt->__cd_out.__cd.__data, &from_start_copy,
			 (const unsigned char *) from_end, NULL,
			 &dummy, 0, 0));

  *from_stop = (wchar_t *) from_start_copy;
  *to_stop = codecvt->__cd_out.__cd.__data[0].__outbuf;

  switch (status)
    {
    case __GCONV_OK:
    case __GCONV_EMPTY_INPUT:
      result = __codecvt_ok;
      break;

    case __GCONV_FULL_OUTPUT:
    case __GCONV_INCOMPLETE_INPUT:
      result = __codecvt_partial;
      break;

    default:
      result = __codecvt_error;
      break;
    }
#else
# ifdef _GLIBCPP_USE_WCHAR_T
  size_t res;
  const char *from_start_copy = (const char *) from_start;
  size_t from_len = from_end - from_start;
  char *to_start_copy = to_start;
  size_t to_len = to_end - to_start;
  res = iconv (codecvt->__cd_out, &from_start_copy, &from_len,
	       &to_start_copy, &to_len);

  if (res == 0 || from_len == 0)
    result = __codecvt_ok;
  else if (to_len < codecvt->__codecvt_do_max_length (codecvt))
    result = __codecvt_partial;
  else
    result = __codecvt_error;

# else
  /* Decide what to do.  */
  result = __codecvt_error;
# endif
#endif

  return result;
}


static enum __codecvt_result
do_unshift (struct _IO_codecvt *codecvt, __mbstate_t *statep,
	    char *to_start, char *to_end, char **to_stop)
{
  enum __codecvt_result result;

#ifdef _LIBC
  struct __gconv_step *gs = codecvt->__cd_out.__cd.__steps;
  int status;
  size_t dummy;

  codecvt->__cd_out.__cd.__data[0].__outbuf = to_start;
  codecvt->__cd_out.__cd.__data[0].__outbufend = to_end;
  codecvt->__cd_out.__cd.__data[0].__statep = statep;

  status = DL_CALL_FCT (gs->__fct,
			(gs, codecvt->__cd_out.__cd.__data, NULL, NULL,
			 NULL, &dummy, 1, 0));

  *to_stop = codecvt->__cd_out.__cd.__data[0].__outbuf;

  switch (status)
    {
    case __GCONV_OK:
    case __GCONV_EMPTY_INPUT:
      result = __codecvt_ok;
      break;

    case __GCONV_FULL_OUTPUT:
    case __GCONV_INCOMPLETE_INPUT:
      result = __codecvt_partial;
      break;

    default:
      result = __codecvt_error;
      break;
    }
#else
# ifdef _GLIBCPP_USE_WCHAR_T
  size_t res;
  char *to_start_copy = (char *) to_start;
  size_t to_len = to_end - to_start;

  res = iconv (codecvt->__cd_out, NULL, NULL, &to_start_copy, &to_len);

  if (res == 0)
    result = __codecvt_ok;
  else if (to_len < codecvt->__codecvt_do_max_length (codecvt))
    result = __codecvt_partial;
  else
    result = __codecvt_error;
# else
  /* Decide what to do.  */
  result = __codecvt_error;
# endif
#endif

  return result;
}


static enum __codecvt_result
do_in (struct _IO_codecvt *codecvt, __mbstate_t *statep,
       const char *from_start, const char *from_end, const char **from_stop,
       wchar_t *to_start, wchar_t *to_end, wchar_t **to_stop)
{
  enum __codecvt_result result;

#ifdef _LIBC
  struct __gconv_step *gs = codecvt->__cd_in.__cd.__steps;
  int status;
  size_t dummy;
  const unsigned char *from_start_copy = (unsigned char *) from_start;

  codecvt->__cd_in.__cd.__data[0].__outbuf = (char *) to_start;
  codecvt->__cd_in.__cd.__data[0].__outbufend = (char *) to_end;
  codecvt->__cd_in.__cd.__data[0].__statep = statep;

  status = DL_CALL_FCT (gs->__fct,
			(gs, codecvt->__cd_in.__cd.__data, &from_start_copy,
			 from_end, NULL, &dummy, 0, 0));

  *from_stop = from_start_copy;
  *to_stop = (wchar_t *) codecvt->__cd_in.__cd.__data[0].__outbuf;

  switch (status)
    {
    case __GCONV_OK:
    case __GCONV_EMPTY_INPUT:
      result = __codecvt_ok;
      break;

    case __GCONV_FULL_OUTPUT:
    case __GCONV_INCOMPLETE_INPUT:
      result = __codecvt_partial;
      break;

    default:
      result = __codecvt_error;
      break;
    }
#else
# ifdef _GLIBCPP_USE_WCHAR_T
  size_t res;
  const char *from_start_copy = (const char *) from_start;
  size_t from_len = from_end - from_start;
  char *to_start_copy = (char *) from_start;
  size_t to_len = to_end - to_start;

  res = iconv (codecvt->__cd_in, &from_start_copy, &from_len,
	       &to_start_copy, &to_len);

  if (res == 0)
    result = __codecvt_ok;
  else if (to_len == 0)
    result = __codecvt_partial;
  else if (from_len < codecvt->__codecvt_do_max_length (codecvt))
    result = __codecvt_partial;
  else
    result = __codecvt_error;
# else
  /* Decide what to do.  */
  result = __codecvt_error;
# endif
#endif

  return result;
}


static int
do_encoding (struct _IO_codecvt *codecvt)
{
#ifdef _LIBC
  /* See whether the encoding is stateful.  */
  if (codecvt->__cd_in.__cd.__steps[0].__stateful)
    return -1;
  /* Fortunately not.  Now determine the input bytes for the conversion
     necessary for each wide character.  */
  if (codecvt->__cd_in.__cd.__steps[0].__min_needed_from
      != codecvt->__cd_in.__cd.__steps[0].__max_needed_from)
    /* Not a constant value.  */
    return 0;

  return codecvt->__cd_in.__cd.__steps[0].__min_needed_from;
#else
  /* Worst case scenario.  */
  return -1;
#endif
}


static int
do_always_noconv (struct _IO_codecvt *codecvt)
{
  return 0;
}


static int
do_length (struct _IO_codecvt *codecvt, __mbstate_t *statep,
	   const char *from_start, const char *from_end, _IO_size_t max)
{
  int result;
#ifdef _LIBC
  const unsigned char *cp = (const unsigned char *) from_start;
  wchar_t to_buf[max];
  struct __gconv_step *gs = codecvt->__cd_in.__cd.__steps;
  int status;
  size_t dummy;

  codecvt->__cd_in.__cd.__data[0].__outbuf = (char *) to_buf;
  codecvt->__cd_in.__cd.__data[0].__outbufend = (char *) &to_buf[max];
  codecvt->__cd_in.__cd.__data[0].__statep = statep;

  status = DL_CALL_FCT (gs->__fct,
			(gs, codecvt->__cd_in.__cd.__data, &cp, from_end,
			 NULL, &dummy, 0, 0));

  result = cp - (const unsigned char *) from_start;
#else
# ifdef _GLIBCPP_USE_WCHAR_T
  const char *from_start_copy = (const char *) from_start;
  size_t from_len = from_end - from_start;
  wchar_t to_buf[max];
  size_t res;
  char *to_start = (char *) to_buf;

  res = iconv (codecvt->__cd_in, &from_start_copy, &from_len,
	       &to_start, &max);

  result = from_start_copy - (char *) from_start;
# else
  /* Decide what to do.  */
  result = 0;
# endif
#endif

  return result;
}


static int
do_max_length (struct _IO_codecvt *codecvt)
{
#ifdef _LIBC
  return codecvt->__cd_in.__cd.__steps[0].__max_needed_from;
#else
  return MB_CUR_MAX;
#endif
}
