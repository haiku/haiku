/* code -- bigram- and front-encode filenames for locate
   Copyright (C) 1994, 2000, 2003, 2004, 2005, 2007 Free Software
   Foundation, Inc.

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

/* Compress a sorted list.
   Works with `find' to encode a filename database to save space
   and search time.

   Usage:

   bigram < file_list > bigrams
   process-bigrams > most_common_bigrams
   code most_common_bigrams < file_list > squeezed_list

   Uses `front compression' (see ";login:", March 1983, p. 8).
   The output begins with the 128 most common bigrams.
   After that, the output format is, for each line,
   an offset (from the previous line) differential count byte
   followed by a (partially bigram-encoded) ASCII remainder.
   The output lines have no terminating byte; the start of the next line
   is indicated by its first byte having a value <= 30.

   The encoding of the output bytes is:

   0-28		likeliest differential counts + offset (14) to make nonnegative
   30		escape code for out-of-range count to follow in next halfword
   128-255	bigram codes (the 128 most common, as determined by `updatedb')
   32-127	single character (printable) ASCII remainder

   Written by James A. Woods <jwoods@adobe.com>.
   Modified by David MacKenzie <djm@gnu.org>.  */

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

#if ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
#else
# define _(Text) Text
#define textdomain(Domain)
#define bindtextdomain(Package, Directory)
#endif
#ifdef gettext_noop
# define N_(String) gettext_noop (String)
#else
/* See locate.c for explanation as to why not use (String) */
# define N_(String) String
#endif

#include "locatedb.h"
#include "closeout.h"
#include "gnulib-version.h"

char *xmalloc PARAMS((size_t));

/* The name this program was run with.  */
char *program_name;

/* The 128 most common bigrams in the file list, padded with NULs
   if there are fewer.  */
static char bigrams[257] = {0};

/* Return the offset of PATTERN in STRING, or -1 if not found. */

static int
strindex (char *string, char *pattern)
{
  register char *s;

  for (s = string; *s != '\0'; s++)
    /* Fast first char check. */
    if (*s == *pattern)
      {
	register char *p2 = pattern + 1, *s2 = s + 1;
	while (*p2 != '\0' && *p2 == *s2)
	  p2++, s2++;
	if (*p2 == '\0')
	  return s2 - strlen (pattern) - string;
      }
  return -1;
}

/* Return the length of the longest common prefix of strings S1 and S2. */

static int
prefix_length (char *s1, char *s2)
{
  register char *start;

  for (start = s1; *s1 == *s2 && *s1 != '\0'; s1++, s2++)
    ;
  return s1 - start;
}

extern char *version_string;

static void
usage (FILE *stream)
{
  fprintf (stream, _("\
Usage: %s [--version | --help]\n\
or     %s most_common_bigrams < file-list > locate-database\n"),
	   program_name, program_name);
  fputs (_("\nReport bugs to <bug-findutils@gnu.org>.\n"), stream);
}


int
main (int argc, char **argv)
{
  char *path;			/* The current input entry.  */
  char *oldpath;		/* The previous input entry.  */
  size_t pathsize, oldpathsize;	/* Amounts allocated for them.  */
  int count, oldcount, diffcount; /* Their prefix lengths & the difference. */
  char bigram[3];		/* Bigram to search for in table.  */
  int code;			/* Index of `bigram' in bigrams table.  */
  FILE *fp;			/* Most common bigrams file.  */
  int line_len;			/* Length of input line.  */

  program_name = argv[0];
  atexit (close_stdout);

  bigram[2] = '\0';

  if (argc != 2)
    {
      usage(stderr);
      return 2;
    }

  if (0 == strcmp(argv[1], "--help"))
    {
      usage(stdout);
      return 0;
    }
  else if (0 == strcmp(argv[1], "--version"))
    {
      printf (_("GNU findutils version %s\n"), version_string);
      printf (_("Built using GNU gnulib version %s\n"), gnulib_version);
      return 0;
    }

  fp = fopen (argv[1], "r");
  if (fp == NULL)
    {
      fprintf (stderr, "%s: ", argv[0]);
      perror (argv[1]);
      return 1;
    }

  pathsize = oldpathsize = 1026; /* Increased as necessary by getline.  */
  path = xmalloc (pathsize);
  oldpath = xmalloc (oldpathsize);

  /* Set to empty string, to force the first prefix count to 0.  */
  oldpath[0] = '\0';
  oldcount = 0;

  /* Copy the list of most common bigrams to the output,
     padding with NULs if there are <128 of them.  */
  fgets (bigrams, 257, fp);
  fwrite (bigrams, 1, 256, stdout);
  fclose (fp);

  while ((line_len = getline (&path, &pathsize, stdin)) > 0)
    {
      char *pp;

      path[line_len - 1] = '\0'; /* Remove newline. */

      /* Squelch unprintable chars in path so as not to botch decoding.  */
      for (pp = path; *pp != '\0'; pp++)
	{
	  if (!(*pp >= 040 && *pp < 0177))
	    *pp = '?';
	}

      count = prefix_length (oldpath, path);
      diffcount = count - oldcount;
      oldcount = count;
      /* If the difference is small, it fits in one byte;
	 otherwise, two bytes plus a marker noting that fact.  */
      if (diffcount < -LOCATEDB_OLD_OFFSET || diffcount > LOCATEDB_OLD_OFFSET)
	{
	  putc (LOCATEDB_OLD_ESCAPE, stdout);
	  putw (diffcount + LOCATEDB_OLD_OFFSET, stdout);
	}
      else
	putc (diffcount + LOCATEDB_OLD_OFFSET, stdout);

      /* Look for bigrams in the remainder of the path.  */
      for (pp = path + count; *pp != '\0'; pp += 2)
	{
	  if (pp[1] == '\0')
	    {
	      /* No bigram is possible; only one char is left.  */
	      putchar (*pp);
	      break;
	    }
	  bigram[0] = *pp;
	  bigram[1] = pp[1];
	  /* Linear search for specific bigram in string table. */
	  code = strindex (bigrams, bigram);
	  if (code % 2 == 0)
	    putchar ((code / 2) | 0200); /* It's a common bigram.  */
	  else
	    fputs (bigram, stdout); /* Write the text as printable ASCII.  */
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
