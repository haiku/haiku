/* listfile.c -- display a long listing of a file
   Copyright (C) 1991, 1993 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>
#include "pathmax.h"

#if defined(HAVE_STRING_H) || defined(STDC_HEADERS)
#include <string.h>
#else
#include <strings.h>
#endif
#ifdef STDC_HEADERS
#include <stdlib.h>
#else
char *getenv ();
extern int errno;
#endif

/* Since major is a function on SVR4, we can't use `ifndef major'.  */
#ifdef MAJOR_IN_MKDEV
#include <sys/mkdev.h>
#define HAVE_MAJOR
#endif
#ifdef MAJOR_IN_SYSMACROS
#include <sys/sysmacros.h>
#define HAVE_MAJOR
#endif

#ifdef STAT_MACROS_BROKEN
#undef S_ISCHR
#undef S_ISBLK
#undef S_ISLNK
#endif

#ifndef S_ISCHR
#define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#endif
#ifndef S_ISBLK
#define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
#endif
#if defined(S_IFLNK) && !defined(S_ISLNK)
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
int readlink ();
#endif

//#if defined(S_ISLNK)
//#endif

/* Extract or fake data from a `struct stat'.
   ST_NBLOCKS: Number of 512-byte blocks in the file
   (including indirect blocks).
   HP-UX, perhaps uniquely, counts st_blocks in 1024-byte units.
   This workaround loses when mixing HP-UX and 4BSD filesystems, though.  */
#ifdef _POSIX_SOURCE
# define ST_NBLOCKS(statp) (((statp)->st_size + 512 - 1) / 512)
#else
# ifndef HAVE_ST_BLOCKS
#  define ST_NBLOCKS(statp) (st_blocks ((statp)->st_size))
# else
#  if defined(hpux) || defined(__hpux__)
#   define ST_NBLOCKS(statp) ((statp)->st_blocks * 2)
#  else
#   define ST_NBLOCKS(statp) ((statp)->st_blocks)
#  endif
# endif
#endif

/* Convert B 512-byte blocks to kilobytes if K is nonzero,
   otherwise return it unchanged. */
#define convert_blocks(b, k) ((k) ? ((b) + 1) / 2 : (b))

#ifndef _POSIX_VERSION
struct passwd *getpwuid ();
struct group *getgrgid ();
#endif

#ifdef major			/* Might be defined in sys/types.h.  */
#define HAVE_MAJOR
#endif
#ifndef HAVE_MAJOR
#define major(dev)  (((dev) >> 8) & 0xff)
#define minor(dev)  ((dev) & 0xff)
#endif
#undef HAVE_MAJOR

char *xmalloc ();
void error ();
void mode_string ();

char *get_link_name ();
char *getgroup ();
char *getuser ();
void print_name_with_quoting ();

/* NAME is the name to print.
   RELNAME is the path to access it from the current directory.
   STATP is the results of stat or lstat on it.
   STREAM is the stdio stream to print on.  */

void
list_file (name, relname, statp, stream)
     char *name;
     char *relname;
     struct stat *statp;
     FILE *stream;
{
  static int kilobytes = -1;	/* -1 = uninitialized, 0 = 512, 1 = 1024. */
  char modebuf[20];
  char timebuf[40];
  time_t current_time = time ((time_t *) 0);

  if (kilobytes == -1)
    kilobytes = getenv ("POSIXLY_CORRECT") == 0;

  mode_string (statp->st_mode, modebuf);
  modebuf[10] = '\0';

  strcpy (timebuf, ctime (&statp->st_mtime));
  if (current_time > statp->st_mtime + 6L * 30L * 24L * 60L * 60L /* Old. */
      || current_time < statp->st_mtime - 60L * 60L) /* In the future. */
    {
      /* The file is fairly old or in the future.
	 POSIX says the cutoff is 6 months old;
	 approximate this by 6*30 days.
	 Allow a 1 hour slop factor for what is considered "the future",
	 to allow for NFS server/client clock disagreement.
	 Show the year instead of the time of day.  */
      strcpy (timebuf + 11, timebuf + 19);
    }
  timebuf[16] = 0;

  fprintf (stream, "%6lu ", statp->st_ino);

  fprintf (stream, "%4u ", convert_blocks (ST_NBLOCKS (statp), kilobytes));

  /* The space between the mode and the number of links is the POSIX
     "optional alternate access method flag". */
  fprintf (stream, "%s %3u ", modebuf, statp->st_nlink);

  fprintf (stream, "%-8.8s ", getuser (statp->st_uid));

  fprintf (stream, "%-8.8s ", getgroup (statp->st_gid));

  if (S_ISCHR (statp->st_mode) || S_ISBLK (statp->st_mode))
#ifdef HAVE_ST_RDEV
    fprintf (stream, "%3u, %3u ", major (statp->st_rdev), minor (statp->st_rdev));
#else
    fprintf (stream, "         ");
#endif
  else
    fprintf (stream, "%8lu ", statp->st_size);

  fprintf (stream, "%s ", timebuf + 4);

  print_name_with_quoting (name, stream);

#ifdef S_ISLNK
  if (S_ISLNK (statp->st_mode))
    {
      char *linkname = get_link_name (name, relname);

      if (linkname)
	{
	  fputs (" -> ", stream);
	  print_name_with_quoting (linkname, stream);
	  free (linkname);
	}
    }
#endif
  putc ('\n', stream);
}

void
print_name_with_quoting (p, stream)
     register char *p;
     FILE *stream;
{
  register unsigned char c;

  while ((c = *p++) != '\0')
    {
      switch (c)
	{
	case '\\':
	  fprintf (stream, "\\\\");
	  break;

	case '\n':
	  fprintf (stream, "\\n");
	  break;

	case '\b':
	  fprintf (stream, "\\b");
	  break;

	case '\r':
	  fprintf (stream, "\\r");
	  break;

	case '\t':
	  fprintf (stream, "\\t");
	  break;

	case '\f':
	  fprintf (stream, "\\f");
	  break;

	case ' ':
	  fprintf (stream, "\\ ");
	  break;

	case '"':
	  fprintf (stream, "\\\"");
	  break;

	default:
	  if (c > 040 && c < 0177)
	    putc (c, stream);
	  else
	    fprintf (stream, "\\%03o", (unsigned int) c);
	}
    }
}

#ifdef S_ISLNK
char *
get_link_name (name, relname)
     char *name;
     char *relname;
{
  register char *linkname;
  register int linklen;

  /* st_size is wrong for symlinks on AIX, and on
     mount points with some automounters.
     So allocate a pessimistic PATH_MAX + 1 bytes.  */
#define LINK_BUF PATH_MAX
  linkname = (char *) xmalloc (LINK_BUF + 1);
  linklen = readlink (relname, linkname, LINK_BUF);
  if (linklen < 0)
    {
      error (0, errno, "%s", name);
      free (linkname);
      return 0;
    }
  linkname[linklen] = '\0';
  return linkname;
}
#endif
