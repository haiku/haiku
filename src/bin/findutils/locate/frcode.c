/* frcode -- front-compress a sorted list
   Copyright (C) 1994, 2003, 2006, 2007 Free Software Foundation, Inc.

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
   Modified by David MacKenzie <djm@gnu.org>.  */

#include <config.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
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
/* We used to use (String) instead of just String, but apparentl;y ISO C
 * doesn't allow this (at least, that's what HP said when someone reported
 * this as a compiler bug).  This is HP case number 1205608192.  See
 * also http://gcc.gnu.org/bugzilla/show_bug.cgi?id=11250 (which references
 * ANSI 3.5.7p14-15).  The Intel icc compiler also rejects constructs
 * like: static const char buf[] = ("string");
 */
# define N_(String) String
#endif


#include "locatedb.h"
#include <getopt.h>
#include "closeout.h"

char *xmalloc PARAMS((size_t));

/* The name this program was run with.  */
char *program_name;

/* Write out a 16-bit int, high byte first (network byte order).  */

static void
put_short (int c, FILE *fp)
{
  /* XXX: The value may be negative, but I think ISO C doesn't
   * guarantee the assumptions we're making here will hold.
   */
  assert(c <= SHRT_MAX);
  assert(c >= SHRT_MIN);
  putc (c >> 8, fp);
  putc (c, fp);
}

/* Return the length of the longest common prefix of strings S1 and S2. */

static int
prefix_length (char *s1, char *s2)
{
  register char *start;
  int limit = INT_MAX;
  for (start = s1; *s1 == *s2 && *s1 != '\0'; s1++, s2++)
    {
      /* Don't emit a prefix length that will not fit into
       * our return type.
       */
      if (0 == --limit)
	break;
    }
  return s1 - start;
}

static struct option const longopts[] =
{
  {"help", no_argument, NULL, 'h'},
  {"version", no_argument, NULL, 'v'},
  {"null", no_argument, NULL, '0'},
  {NULL, no_argument, NULL, 0}
};

extern char *version_string;

/* The name this program was run with. */
char *program_name;


static void
usage (FILE *stream)
{
  fprintf (stream,
	   _("Usage: %s [-0 | --null] [--version] [--help]\n"),
	   program_name);
  fputs (_("\nReport bugs to <bug-findutils@gnu.org>.\n"), stream);
}


int
main (int argc, char **argv)
{
  char *path;			/* The current input entry.  */
  char *oldpath;		/* The previous input entry.  */
  size_t pathsize, oldpathsize;	/* Amounts allocated for them.  */
  int count, oldcount, diffcount; /* Their prefix lengths & the difference. */
  int line_len;			/* Length of input line.  */
  int delimiter = '\n';
  int optc;

  program_name = argv[0];
  atexit (close_stdout);

  pathsize = oldpathsize = 1026; /* Increased as necessary by getline.  */
  path = xmalloc (pathsize);
  oldpath = xmalloc (oldpathsize);

  oldpath[0] = 0;
  oldcount = 0;


  while ((optc = getopt_long (argc, argv, "hv0", longopts, (int *) 0)) != -1)
    switch (optc)
      {
      case '0':
	delimiter = 0;
	break;

      case 'h':
	usage (stdout);
	return 0;

      case 'v':
	printf (_("GNU locate version %s\n"), version_string);
	return 0;

      default:
	usage (stderr);
	return 1;
      }

  /* We expect to have no arguments. */
  if (optind != argc)
    {
      usage (stderr);
      return 1;
    }



  fwrite (LOCATEDB_MAGIC, sizeof (LOCATEDB_MAGIC), 1, stdout);

  while ((line_len = getdelim (&path, &pathsize, delimiter, stdin)) > 0)
    {
      path[line_len - 1] = '\0'; /* FIXME temporary: nuke the newline.  */

      count = prefix_length (oldpath, path);
      diffcount = count - oldcount;
      if ( (diffcount > SHRT_MAX) || (diffcount < SHRT_MIN) )
	{
	  /* We do this to prevent overflow of the value we
	   * write with put_short()
	   */
	  count = 0;
	  diffcount = (-oldcount);
	}
      oldcount = count;

      /* If the difference is small, it fits in one byte;
	 otherwise, two bytes plus a marker noting that fact.  */
      if (diffcount < LOCATEDB_ONEBYTE_MIN || diffcount > LOCATEDB_ONEBYTE_MAX)
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

  return 0;
}
