/* yesno.c -- read a yes/no response from stdin

   Copyright (C) 1990, 1998, 2001, 2003, 2004, 2005, 2006 Free
   Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include <config.h>

#include "yesno.h"

#include <stdlib.h>
#include <stdio.h>

#if ENABLE_NLS
# include "getline.h"
#endif

/* Return true if we read an affirmative line from standard input.  */

extern int rpmatch (char const *response);

bool
yesno (void)
{
  bool yes;

#if ENABLE_NLS
  char *response = NULL;
  size_t response_size = 0;
  ssize_t response_len = getline (&response, &response_size, stdin);

  if (response_len <= 0)
    yes = false;
  else
    {
      response[response_len - 1] = '\0';
      yes = (0 < rpmatch (response));
    }

  free (response);
#else
  /* Test against "^[yY]", hardcoded to avoid requiring getline,
     regex, and rpmatch.  */
  int c = getchar ();
  yes = (c == 'y' || c == 'Y');
  while (c != '\n' && c != EOF)
    c = getchar ();
#endif

  return yes;
}
