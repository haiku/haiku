/* Return the next element of a path.
   Copyright (C) 1992 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Written by David MacKenzie <djm@gnu.ai.mit.edu>,
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

char *strdup ();
void free ();

/* Return the next element of a colon-separated path.
   A null entry in the path is equivalent to "." (the current directory).

   If NEW_PATH is non-NULL, set the path and return NULL.
   If NEW_PATH is NULL, return the next item in the string, or
   return NULL if there are no more elements.  */

char *
next_element (new_path)
     char *new_path;
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
	  return ".";
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
      return ".";
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
