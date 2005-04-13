/* Copyright (C) 1991, 1992, 1995-1999, 2002 Free Software Foundation, Inc.
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
   02111-1307 USA.  */

#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <gconv.h>
#include <wcsmbs/wcsmbsload.h>


extern mbstate_t __no_r_state attribute_hidden;	/* Defined in mbtowc.c.  */

/* Convert WCHAR into its multibyte character representation,
   putting this in S and returning its length.

   Attention: this function should NEVER be intentionally used.
   The interface is completely stupid.  The state is shared between
   all conversion functions.  You should use instead the restartable
   version `wcrtomb'.  */
int
wctomb (char *s, wchar_t wchar)
{
  /* If S is NULL the function has to return null or not null
     depending on the encoding having a state depending encoding or
     not.  */
  if (s == NULL)
    {
      const struct gconv_fcts *fcts;

      /* Get the conversion functions.  */
      fcts = get_gconv_fcts (_NL_CURRENT_DATA (LC_CTYPE));

      /* This is an extension in the Unix standard which does not directly
	 violate ISO C.  */
      memset (&__no_r_state, '\0', sizeof __no_r_state);

      return fcts->tomb->__stateful;
    }

  return __wcrtomb (s, wchar, &__no_r_state);
}
libc_hidden_def (wctomb)
