/* Return the next element of a path.
   Copyright (C) 1992, 2003, 2004, 2005 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* Written by David MacKenzie <djm@gnu.org>,
   inspired by John P. Rouillard <rouilj@cs.umb.edu>.  */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#if defined(HAVE_STRING_H) || defined(STDC_HEADERS)
#include <string.h>
#else
#include <strings.h>
#ifndef strchr
#define strchr index
#endif
#endif
#if defined(STDC_HEADERS)
#include <stdlib.h>
#endif

#include "nextelem.h"


/* Return the next element of a colon-separated path.
   A null entry in the path is equivalent to "." (the current directory).

   If NEW_PATH is non-NULL, set the path and return NULL.
   If NEW_PATH is NULL, return the next item in the string, or
   return NULL if there are no more elements.  */

char *
next_element (const char *new_path, int curdir_ok)
{
  static char *path = NULL;	/* Freshly allocated copy of NEW_PATH.  */
  static char *end;		/* Start of next element to return.  */
  static int final_colon;	/* If zero, path didn't end with a colon.  */
  char *start;			/* Start of path element to return.  */

  if (new_path)
    {
      if (path)
	free (path);
      end = path = strdup (new_path);
      final_colon = 0;
      return NULL;
    }

  if (*end == '\0')
    {
      if (final_colon)
	{
	  final_colon = 0;
	  return curdir_ok ? "." : "";
	}
      return NULL;
    }

  start = end;
  final_colon = 1;		/* Maybe there will be one.  */

  end = strchr (start, ':');
  if (end == start)
    {
      /* An empty path element.  */
      *end++ = '\0';
      return curdir_ok ? "." : "";
    }
  else if (end == NULL)
    {
      /* The last path element.  */
      end = strchr (start, '\0');
      final_colon = 0;
    }
  else
    *end++ = '\0';

  return start;
}

#ifdef TEST
int
main ()
{
  char *p;

  next_element (getenv ("PATH"));
  while (p = next_element (NULL))
    puts (p);
  exit (0);
}
#endif /* TEST */
