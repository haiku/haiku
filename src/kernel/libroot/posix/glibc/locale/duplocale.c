/* Duplicate handle for selection of locales.
   Copyright (C) 1997, 2000, 2001 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1997.

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

#include <locale.h>
#include <bits/libc-lock.h>
#include <stdlib.h>

#include <localeinfo.h>


/* Lock for protecting global data.  */
__libc_lock_define (extern , __libc_setlocale_lock)


__locale_t
__duplocale (__locale_t dataset)
{
  __locale_t result;

  /* We modify global data.  */
  __libc_lock_lock (__libc_setlocale_lock);

  /* Get memory.  */
  result = (__locale_t) malloc (sizeof (struct __locale_struct));
  if (result != NULL)
    {
      int cnt;
      for (cnt = 0; cnt < __LC_LAST; ++cnt)
	if (cnt != LC_ALL)
	  {
	    result->__locales[cnt] = dataset->__locales[cnt];
	    if (result->__locales[cnt]->usage_count < MAX_USAGE_COUNT)
	      ++result->__locales[cnt]->usage_count;
	  }
    }

  /* Update the special members.  */
  result->__ctype_b = dataset->__ctype_b;
  result->__ctype_tolower = dataset->__ctype_tolower;
  result->__ctype_toupper = dataset->__ctype_toupper;

  /* It's done.  */
  __libc_lock_unlock (__libc_setlocale_lock);

  return result;
}
