/* Test of conversion of unibyte character to wide character.
   Copyright (C) 2008-2011 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2008.  */

#include <assert.h>
#include <locale.h>
#include <stdio.h>
#include <wchar.h>

int
main (int argc, char *argv[])
{
  int c, i;

  /* configure should already have checked that the locale is supported.  */
  if (setlocale (LC_ALL, "") == NULL) {
	fprintf(stderr, "unable to set standard locale\n");
    return 1;
  }

  assert (btowc (EOF) == WEOF);

  for (i = '1'; i <= '2'; ++i) {
    switch (i)
      {
      case '1':
        /* Locale encoding is ISO-8859-1 or ISO-8859-15.  */
    	printf("ISO8859-1 ...\n");

       	if (setlocale (LC_ALL, "en_US.ISO8859-1") == NULL) {
       	  fprintf(stderr, "unable to set ISO8859-1 locale, skipping\n");
       	  break;
       	}

       	for (c = 0; c < 0x80; c++)
          assert (btowc (c) == (wint_t)c);
        for (c = 0xA0; c < 0x100; c++)
          assert (btowc (c) != WEOF);
        break;

      case '2':
        /* Locale encoding is UTF-8.  */
    	printf("UTF-8 ...\n");

       	if (setlocale (LC_ALL, "en_US.utf-8") == NULL) {
       	  fprintf(stderr, "unable to set en_US.UTF-8 locale, skipping\n");
       	  break;
       	}

    	for (c = 0; c < 0x80; c++)
          assert (btowc (c) == (wint_t)c);
        for (c = 0x80; c < 0x100; c++)
        	assert (btowc (c) == WEOF);
        break;
      }
  }

  return 0;
}
