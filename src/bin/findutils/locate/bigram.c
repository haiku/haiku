/* bigram -- list bigrams for locate
   Copyright (C) 1994, 2000, 2003, 2004, 2005, 2007 Free Software Foundation, Inc.

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

/* Usage: bigram < text > bigrams
   Use `code' to encode a file using this output.

   Read a file from stdin and write out the bigrams (pairs of
   adjacent characters), one bigram per line, to stdout.  To reduce
   needless duplication in the output, it starts finding the
   bigrams on each input line at the character where that line
   first differs from the previous line (i.e., in the ASCII
   remainder).  Therefore, the input should be sorted in order to
   get the least redundant output.

   Written by James A. Woods <jwoods@adobe.com>.
   Modified by David MacKenzie <djm@gnu.ai.mit.edu>.  */

#include <config.h>
#include <stdio.h>

#if defined(HAVE_STRING_H) || defined(STDC_HEADERS)
#include <string.h>
#else
#include <strings.h>
#endif

#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#include <sys/types.h>

#include <xalloc.h>
#include "closeout.h"

/* The name this program was run with.  */
char *program_name;

/* Return the length of the longest common prefix of strings S1 and S2. */

static int
prefix_length (char *s1, char *s2)
{
  register char *start;

  for (start = s1; *s1 == *s2 && *s1 != '\0'; s1++, s2++)
    ;
  return s1 - start;
}

int
main (int argc, char **argv)
{
  char *path;			/* The current input entry.  */
  char *oldpath;		/* The previous input entry.  */
  size_t pathsize, oldpathsize;	/* Amounts allocated for them.  */
  int line_len;			/* Length of input line.  */

  program_name = argv[0];
  (void) argc;
  atexit (close_stdout);

  pathsize = oldpathsize = 1026; /* Increased as necessary by getline.  */
  path = xmalloc (pathsize);
  oldpath = xmalloc (oldpathsize);

  /* Set to empty string, to force the first prefix count to 0.  */
  oldpath[0] = '\0';

  while ((line_len = getline (&path, &pathsize, stdin)) > 0)
    {
      register int count;	/* The prefix length.  */
      register int j;		/* Index into input line.  */

      path[line_len - 1] = '\0'; /* Remove the newline. */

      /* Output bigrams in the remainder only. */
      count = prefix_length (oldpath, path);
      for (j = count; path[j] != '\0' && path[j + 1] != '\0'; j += 2)
	{
	  putchar (path[j]);
	  putchar (path[j + 1]);
	  putchar ('\n');
	}

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

  return 0;
}
