/* Copyright (C) 1996, 1997, 1998, 1999, 2000 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@gnu.org>, 1996.

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
   02111-1307 USA.  */

#include <dlfcn.h>
#include <errno.h>
#include <gconv.h>
#include <wchar.h>
#include <wcsmbsload.h>

#include <assert.h>

#ifndef EILSEQ
# define EILSEQ EINVAL
#endif

/* This is the private state used if PS is NULL.  */
static mbstate_t state;

size_t
__mbrtowc (wchar_t *pwc, const char *s, size_t n, mbstate_t *ps)
{
  wchar_t buf[1];
  struct __gconv_step_data data;
  int status;
  size_t result;
  size_t dummy;
  const unsigned char *inbuf;
  char *outbuf = (char *) (pwc ?: buf);

  /* Set information for this step.  */
  data.__invocation_counter = 0;
  data.__internal_use = 1;
  data.__flags = __GCONV_IS_LAST;
  data.__statep = ps ?: &state;
  data.__trans = NULL;

  /* A first special case is if S is NULL.  This means put PS in the
     initial state.  */
  if (s == NULL)
    {
      outbuf = (char *) buf;
      s = "";
      n = 1;
    }

  /* Tell where we want the result.  */
  data.__outbuf = outbuf;
  data.__outbufend = outbuf + sizeof (wchar_t);

  /* Make sure we use the correct function.  */
  update_conversion_ptrs ();

  /* Do a normal conversion.  */
  inbuf = (const unsigned char *) s;
  status = DL_CALL_FCT (__wcsmbs_gconv_fcts.towc->__fct,
			(__wcsmbs_gconv_fcts.towc, &data, &inbuf, inbuf + n,
			 NULL, &dummy, 0, 1));

  /* There must not be any problems with the conversion but illegal input
     characters.  The output buffer must be large enough, otherwise the
     definition of MB_CUR_MAX is not correct.  All the other possible
     errors also must not happen.  */
  assert (status == __GCONV_OK || status == __GCONV_EMPTY_INPUT
	  || status == __GCONV_ILLEGAL_INPUT
	  || status == __GCONV_INCOMPLETE_INPUT
	  || status == __GCONV_FULL_OUTPUT);

  if (status == __GCONV_OK || status == __GCONV_EMPTY_INPUT
      || status == __GCONV_FULL_OUTPUT)
    {
      if (data.__outbuf != (unsigned char *) outbuf
	  && *(wchar_t *) outbuf == L'\0')
	{
	  /* The converted character is the NUL character.  */
	  assert (__mbsinit (data.__statep));
	  result = 0;
	}
      else
	result = inbuf - (const unsigned char *) s;
    }
  else if (status == __GCONV_INCOMPLETE_INPUT)
    result = (size_t) -2;
  else
    {
      result = (size_t) -1;
      __set_errno (EILSEQ);
    }

  return result;
}
weak_alias (__mbrtowc, mbrtowc)
