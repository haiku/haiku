/* locate -- search databases for filenames that match patterns
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

/* Usage: locate [options] pattern...

   Scan a pathname list for the full pathname of a file, given only
   a piece of the name (possibly containing shell globbing metacharacters).
   The list has been processed with front-compression, which reduces
   the list size by a factor of 4-5.
   Recognizes two database formats, old and new.  The old format is
   bigram coded, which reduces space by a further 20-25% and uses the
   following encoding of the database bytes:

   0-28		likeliest differential counts + offset (14) to make nonnegative
   30		escape code for out-of-range count to follow in next halfword
   128-255 	bigram codes (the 128 most common, as determined by `updatedb')
   32-127  	single character (printable) ASCII remainder

   Uses a novel two-tiered string search technique:

   First, match a metacharacter-free subpattern and a partial pathname
   BACKWARDS to avoid full expansion of the pathname list.
   The time savings is 40-50% over forward matching, which cannot efficiently
   handle overlapped search patterns and compressed path remainders.

   Then, match the actual shell glob-style regular expression (if in this form)
   against the candidate pathnames using the slower shell filename
   matching routines.

   Described more fully in Usenix ;login:, Vol 8, No 1,
   February/March, 1983, p. 8.

   Written by James A. Woods <jwoods@adobe.com>.
   Modified by David MacKenzie <djm@gnu.ai.mit.edu>.  */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <fnmatch.h>
#include <getopt.h>

#define NDEBUG
#include <assert.h>

#if defined(HAVE_STRING_H) || defined(STDC_HEADERS)
#include <string.h>
#else
#include <strings.h>
#define strchr index
#endif

#ifdef STDC_HEADERS
#include <stdlib.h>
#else
char *getenv ();
#endif

#ifdef STDC_HEADERS
#include <errno.h>
#include <stdlib.h>
#else
extern int errno;
#endif

#include "locatedb.h"

typedef enum {false, true} boolean;

/* Warn if a database is older than this.  8 days allows for a weekly
   update that takes up to a day to perform.  */
#define WARN_SECONDS (60 * 60 * 24 * 8)

/* Printable version of WARN_SECONDS.  */
#define WARN_MESSAGE "8 days"

char *next_element ();
char *xmalloc ();
char *xrealloc ();
void error ();

#ifndef HAVE_GETW
int
getw(fp)
		FILE *fp;
{
		int x;
		
		return (fread((void*)&x, sizeof(x), 1, fp) == 1 ? x : EOF);
}
#endif

/* Read in a 16-bit int, high byte first (network byte order).  */

static int
get_short (fp)
     FILE *fp;
{
  register short x;

  x = fgetc (fp);
  return (x << 8) | (fgetc (fp) & 0xff);
}

/* Return a pointer to the last character in a static copy of the last
   glob-free subpattern in NAME,
   with '\0' prepended for a fast backwards pre-match.  */

static char *
last_literal_end (name)
     char *name;
{
  static char *globfree = NULL;	/* A copy of the subpattern in NAME.  */
  static size_t gfalloc = 0;	/* Bytes allocated for `globfree'.  */
  register char *subp;		/* Return value.  */
  register char *p;		/* Search location in NAME.  */

  /* Find the end of the subpattern.
     Skip trailing metacharacters and [] ranges. */
  for (p = name + strlen (name) - 1; p >= name && strchr ("*?]", *p) != NULL;
       p--)
    {
      if (*p == ']')
	while (p >= name && *p != '[')
	  p--;
    }
  if (p < name)
    p = name;

  if (p - name + 3 > gfalloc)
    {
      gfalloc = p - name + 3 + 64; /* Room to grow.  */
      globfree = xrealloc (globfree, gfalloc);
    }
  subp = globfree;
  *subp++ = '\0';

  /* If the pattern has only metacharacters, make every path match the
     subpattern, so it gets checked the slow way.  */
  if (p == name && strchr ("?*[]", *p) != NULL)
    *subp++ = '/';
  else
    {
      char *endmark;
      /* Find the start of the metacharacter-free subpattern.  */
      for (endmark = p; p >= name && strchr ("]*?", *p) == NULL; p--)
	;
      /* Copy the subpattern into globfree.  */
      for (++p; p <= endmark; )
	*subp++ = *p++;
    }
  *subp-- = '\0';		/* Null terminate, though it's not needed.  */

  return subp;
}

/* Print the entries in DBFILE that match shell globbing pattern PATHPART.
   Return the number of entries printed.  */

static int
locate (pathpart, dbfile)
     char *pathpart, *dbfile;
{
  /* The pathname database.  */
  FILE *fp;
  /* An input byte.  */
  int c;
  /* Number of bytes read from an entry.  */
  int nread;

  /* true if PATHPART contains globbing metacharacters.  */
  boolean globflag;
  /* The end of the last glob-free subpattern in PATHPART.  */
  char *patend;

  /* The current input database entry.  */
  char *path;
  /* Amount allocated for it.  */
  size_t pathsize;

  /* The length of the prefix shared with the previous database entry.  */
  int count = 0;
  /* Where in `path' to stop the backward search for the last character
     in the subpattern.  Set according to `count'.  */
  char *cutoff;

  /* true if we found a fast match (of patend) on the previous path.  */
  boolean prev_fast_match = false;
  /* The return value.  */
  int printed = 0;

  /* true if reading a bigram-encoded database.  */
  boolean old_format = false;
  /* For the old database format,
     the first and second characters of the most common bigrams.  */
  char bigram1[128], bigram2[128];

  /* To check the age of the database.  */
  struct stat st;
  time_t now;

  if (stat (dbfile, &st) || (fp = fopen (dbfile, "r")) == NULL)
    {
      error (0, errno, "%s", dbfile);
      return 0;
    }
  time(&now);
  if (now - st.st_mtime > WARN_SECONDS)
    {
      error (0, 0, "warning: database `%s' is more than %s old",
	     dbfile, WARN_MESSAGE);
    }

  pathsize = 1026;		/* Increased as necessary by getstr.  */
  path = xmalloc (pathsize);

  nread = fread (path, 1, sizeof (LOCATEDB_MAGIC), fp);
  if (nread != sizeof (LOCATEDB_MAGIC)
      || memcmp (path, LOCATEDB_MAGIC, sizeof (LOCATEDB_MAGIC)))
    {
      int i;
      /* Read the list of the most common bigrams in the database.  */
      fseek (fp, 0, 0);
      for (i = 0; i < 128; i++)
	{
	  bigram1[i] = getc (fp);
	  bigram2[i] = getc (fp);
	}
      old_format = true;
    }

  globflag = strchr (pathpart, '*') || strchr (pathpart, '?')
    || strchr (pathpart, '[');

  patend = last_literal_end (pathpart);

  c = getc (fp);
  while (c != EOF)
    {
      register char *s;		/* Scan the path we read in.  */

      if (old_format)
	{
	  /* Get the offset in the path where this path info starts.  */
	  if (c == LOCATEDB_OLD_ESCAPE)
	    count += getw (fp) - LOCATEDB_OLD_OFFSET;
	  else
	    count += c - LOCATEDB_OLD_OFFSET;

	  /* Overlay the old path with the remainder of the new.  */
	  for (s = path + count; (c = getc (fp)) > LOCATEDB_OLD_ESCAPE;)
	    if (c < 0200)
	      *s++ = c;		/* An ordinary character.  */
	    else
	      {
		/* Bigram markers have the high bit set. */
		c &= 0177;
		*s++ = bigram1[c];
		*s++ = bigram2[c];
	      }
	  *s-- = '\0';
	}
      else
	{
	  if (c == LOCATEDB_ESCAPE)
	    count += get_short (fp);
	  else if (c > 127)
	    count += c - 256;
	  else
	    count += c;

	  /* Overlay the old path with the remainder of the new.  */
	  nread = getstr (&path, &pathsize, fp, '\0', count);
	  if (nread < 0)
	    break;
	  c = getc (fp);
	  s = path + count + nread - 2; /* Move to the last char in path.  */
	  assert (s[0] != '\0');
	  assert (s[1] == '\0'); /* Our terminator.  */
	  assert (s[2] == '\0'); /* Added by getstr.  */
	}

      /* If the previous path matched, scan the whole path for the last
	 char in the subpattern.  If not, the shared prefix doesn't match
	 the pattern, so don't scan it for the last char.  */
      cutoff = prev_fast_match ? path : path + count;

      /* Search backward starting at the end of the path we just read in,
	 for the character at the end of the last glob-free subpattern
	 in PATHPART.  */
      for (prev_fast_match = false; s >= cutoff; s--)
	/* Fast first char check. */
	if (*s == *patend)
	  {
	    char *s2;		/* Scan the path we read in. */
	    register char *p2;	/* Scan `patend'.  */

	    for (s2 = s - 1, p2 = patend - 1; *p2 != '\0' && *s2 == *p2;
					       s2--, p2--)
	      ;
	    if (*p2 == '\0')
	      {
		/* Success on the fast match.  Compare the whole pattern
		   if it contains globbing characters.  */
		prev_fast_match = true;
		if (globflag == false || fnmatch (pathpart, path, 0) == 0)
		  {
		    puts (path);
		    ++printed;
		  }
		break;
	      }
	  }
    }

  if (ferror (fp))
    {
      error (0, errno, "%s", dbfile);
      return 0;
    }
  if (fclose (fp) == EOF)
    {
      error (0, errno, "%s", dbfile);
      return 0;
    }

  return printed;
}

extern char *version_string;

/* The name this program was run with. */
char *program_name;

static void
usage (stream, status)
     FILE *stream;
     int status;
{
  fprintf (stream, "\
Usage: %s [-d path] [--database=path] [--version] [--help] pattern...\n",
	   program_name);
  exit (status);
}

static struct option const longopts[] =
{
  {"database", required_argument, NULL, 'd'},
  {"help", no_argument, NULL, 'h'},
  {"version", no_argument, NULL, 'v'},
  {NULL, no_argument, NULL, 0}
};

void
main (argc, argv)
     int argc;
     char **argv;
{
  char *dbpath;
  int found = 0, optc;

  program_name = argv[0];

  dbpath = getenv ("LOCATE_PATH");
  if (dbpath == NULL)
    dbpath = LOCATE_DB;

  while ((optc = getopt_long (argc, argv, "d:", longopts, (int *) 0)) != -1)
    switch (optc)
      {
      case 'd':
	dbpath = optarg;
	break;

      case 'h':
	usage (stdout, 0);

      case 'v':
	printf ("GNU locate version %s\n", version_string);
	exit (0);

      default:
	usage (stderr, 1);
      }

  if (optind == argc)
    usage (stderr, 1);

  for (; optind < argc; optind++)
    {
      char *e;
      next_element (dbpath);	/* Initialize.  */
      while ((e = next_element ((char *) NULL)) != NULL)
	found |= locate (argv[optind], e);
    }

  exit (!found);
}
