/* Copyright (C) 1996,1997,1998,1999,2000,2002 Free Software Foundation, Inc.
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
#include <gconv.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <wcsmbsload.h>
#include <limits.h>


wint_t
__btowc (c)
     int c;
{
  const struct gconv_fcts *fcts;

  /* If the parameter does not fit into one byte or it is the EOF value
     we can give the answer now.  */
  if (c < SCHAR_MIN || c > UCHAR_MAX || c == EOF)
    return WEOF;

  /* Get the conversion functions.  */
  fcts = get_gconv_fcts (_NL_CURRENT_DATA (LC_CTYPE));

  if (__builtin_expect (fcts->towc_nsteps == 1, 1)
      && __builtin_expect (fcts->towc->__btowc_fct != NULL, 1))
    {
      /* Use the shortcut function.  */
      return DL_CALL_FCT (fcts->towc->__btowc_fct,
			  (fcts->towc, (unsigned char) c));
    }
  else
    {
      /* Fall back to the slow but generic method.  */
      wchar_t result;
      struct __gconv_step_data data;
      unsigned char inbuf[1];
      const unsigned char *inptr = inbuf;
      size_t dummy;
      int status;

      /* Tell where we want the result.  */
      data.__outbuf = (unsigned char *) &result;
      data.__outbufend = data.__outbuf + sizeof (wchar_t);
      data.__invocation_counter = 0;
      data.__internal_use = 1;
      data.__flags = __GCONV_IS_LAST;
      data.__statep = &data.__state;
      data.__trans = NULL;

      /* Make sure we start in the initial state.  */
      memset (&data.__state, '\0', sizeof (mbstate_t));

      /* Create the input string.  */
      inbuf[0] = c;

      status = DL_CALL_FCT (fcts->towc->__fct,
			    (fcts->towc, &data, &inptr, inptr + 1,
			     NULL, &dummy, 0, 1));

      if (status != __GCONV_OK && status != __GCONV_FULL_OUTPUT
	  && status != __GCONV_EMPTY_INPUT)
	/* The conversion failed.  */
	result = WEOF;

      return result;
    }
}
weak_alias (__btowc, btowc)
