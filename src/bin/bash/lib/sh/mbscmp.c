/* mbscmp - multibyte string comparison. */

/* Copyright (C) 1995 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Bash is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <config.h>

#if !defined (HAVE_MBSCMP) && defined (HANDLE_MULTIBYTE)

#include <stdlib.h>
#include <stddef.h>

/* Compare MBS1 and MBS2.  */
int
mbscmp (mbs1, mbs2)
    const char *mbs1;
    const char *mbs2;
{
  int len1, len2;
  wchar_t c1, c2;

  len1 = len2 = 0;
  /* Reset multibyte characters to their initial state.	 */
  (void) mblen ((char *) NULL, 0);

  do
    {
      len1 = mbtowc ((wchar_t *) &c1, mbs1, MB_CUR_MAX);
      len2 = mbtowc ((wchar_t *) &c2, mbs2, MB_CUR_MAX);

      if (len1 == 0)
	return len2 == 0 ? 0 : -1;
      if (len2 == 0)
	return 1;
      if (len1 < 0 || len2 < 0)
	/* FIXME: an illegal character appears.	 What to do?  */
	return c1 - c2;

      mbs1 += len1;
      mbs2 += len2;
    }
  while (c1 == c2);

  return c1 - c2;
}

#endif
