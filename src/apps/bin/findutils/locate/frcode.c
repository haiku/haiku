/* frcode -- front-compress a sorted list
   Copyright (C) 1994 Free Software Foundation, Inc.

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

/* Usage: frcode < sorted-list > compressed-list

   Uses front compression (also known as incremental encoding);
   see ";login:", March 1983, p. 8.

   The input is a sorted list of NUL-terminated strings.
   (FIXME newline-terminated, until we figure out how to sort
   NUL-terminated strings.)

   The output entries are in the same order as the input;
   each entry consists of an offset-differential count byte
   (the additional number of characters of prefix of the preceding entry to
   use beyond the number that the preceding entry is using of its predecessor),
   followed by a null-terminated ASCII remainder.

   If the offset-differential count is larger than can be stored
   in a byte (+/-127), the byte has the value LOCATEDB_ESCAPE
   and the count follows in a 2-byte word, with the high byte first
   (network byte order).

   Example:

   Input, with NULs changed to newlines:
   /usr/src
   /usr/src/cmd/aardvark.c
   /usr/src/cmd/armadillo.c
   /usr/tmp/zoo

   Length of the longest prefix of the preceding entry to share:
   0 /usr/src
   8 /cmd/aardvark.c
   14 rmadillo.c
   5 tmp/zoo

   Output, with NULs changed to newlines and count bytes made printable:
   0 LOCATE02
   0 /usr/src
   8 /cmd/aardvark.c
   6 rmadillo.c
   -9 tmp/zoo

   (6 = 14 - 8, and -9 = 5 - 14)

   Written by James A. Woods <jwoods@adobe.com>.
   Modified by David MacKenzie <djm@gnu.ai.mit.edu>.  */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>

#if defined(HAVE_STRING_H) || defined(STDC_HEADERS)
#include <string.h>
#else
#include <strings.h>
#endif

#ifdef STDC_HEADERS
#include <stdlib.h>
#endif

#include "locatedb.h"

char *xmalloc ();

/* The name this program was run with.  */
char *program_name;

/* Write out a 16-bit int, high byte first (network byte order).  */

static void
put_short (c, fp)
     int c;
     FILE *fp;
{
  putc (c >> 8, fp);
  putc (c, fp); 
}

/* Return the length of the longest common prefix of strings S1 and S2. */

static int
prefix_length (s1, s2)
     char *s1, *s2;
{
  register char *start;

  for (start = s1; *s1 == *s2 && *s1 != '\0'; s1++, s2++)
    ;
  return s1 - start;
}

void
main (argc, argv)
     int argc;
     char **argv;
{
  char *path;			/* The current input entry.  */
  char *oldpath;		/* The previous input entry.  */
  size_t pathsize, oldpathsize;	/* Amounts allocated for them.  */
  int count, oldcount, diffcount; /* Their prefix lengths & the difference. */
  int line_len;			/* Length of input line.  */

  program_name = argv[0];

  pathsize = oldpathsize = 1026; /* Increased as necessary by getstr.  */
  path = xmalloc (pathsize);
  oldpath = xmalloc (oldpathsize);

  /* Set to anything not starting with a slash, to force the first
     prefix count to 0.  */
  strcpy (oldpath, " ");
  oldcount = 0;

  fwrite (LOCATEDB_MAGIC, sizeof (LOCATEDB_MAGIC), 1, stdout);

  /* FIXME temporary: change the \n to \0 when we figure out how to sort
     null-terminated strings.  */
  while ((line_len = getstr (&path, &pathsize, stdin, '\n', 0)) > 0)
    {
      path[line_len - 1] = '\0'; /* FIXME temporary: nuke the newline.  */

      count = prefix_length (oldpath, path);
      diffcount = count - oldcount;
      oldcount = count;
      /* If the difference is small, it fits in one byte;
	 otherwise, two bytes plus a marker noting that fact.  */
      if (diffcount < -127 || diffcount > 127)
	{
	  putc (LOCATEDB_ESCAPE, stdout);
	  put_short (diffcount, stdout);
	}
      else
	putc (diffcount, stdout);

      fputs (path + count, stdout);
      putc ('\0', stdout);

      {
	/* Swap path and oldpath and their sizes.  */
	char *tmppath = oldpath;
	size_t tmppathsize = oldpathsize;
	oldpath = path;
	oldpathsize = pathsize;
	path = tmppath;
	pathsize = tmppathsize;
      }
    }

  free (path);
  free (oldpath);

  exit (0);
}
