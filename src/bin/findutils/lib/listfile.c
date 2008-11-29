/* listfile.c -- display a long listing of a file
   Copyright (C) 1991, 1993, 2000, 2003, 2004, 2007 Free Software
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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <alloca.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>
#include "human.h"
#include "xalloc.h"
#include "pathmax.h"
#include "error.h"
#include "filemode.h"
#include "idcache.h"

#include "listfile.h"

#if HAVE_STRING_H || STDC_HEADERS
#include <string.h>
#else
#include <strings.h>
#endif


/* The presence of unistd.h is assumed by gnulib these days, so we
 * might as well assume it too.
 */
#include <unistd.h> /* for readlink() */


#if STDC_HEADERS
# include <stdlib.h>
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
#endif

/* Get or fake the disk device blocksize.
   Usually defined by sys/param.h (if at all).  */
#ifndef DEV_BSIZE
# ifdef BSIZE
#  define DEV_BSIZE BSIZE
# else /* !BSIZE */
#  define DEV_BSIZE 4096
# endif /* !BSIZE */
#endif /* !DEV_BSIZE */

/* Extract or fake data from a `struct stat'.
   ST_BLKSIZE: Preferred I/O blocksize for the file, in bytes.
   ST_NBLOCKS: Number of blocks in the file, including indirect blocks.
   ST_NBLOCKSIZE: Size of blocks used when calculating ST_NBLOCKS.  */
#ifndef HAVE_STRUCT_STAT_ST_BLOCKS
# define ST_BLKSIZE(statbuf) DEV_BSIZE
# if defined(_POSIX_SOURCE) || !defined(BSIZE) /* fileblocks.c uses BSIZE.  */
#  define ST_NBLOCKS(statbuf) \
  (S_ISREG ((statbuf).st_mode) \
   || S_ISDIR ((statbuf).st_mode) \
   ? (statbuf).st_size / ST_NBLOCKSIZE + ((statbuf).st_size % ST_NBLOCKSIZE != 0) : 0)
# else /* !_POSIX_SOURCE && BSIZE */
#  define ST_NBLOCKS(statbuf) \
  (S_ISREG ((statbuf).st_mode) \
   || S_ISDIR ((statbuf).st_mode) \
   ? st_blocks ((statbuf).st_size) : 0)
# endif /* !_POSIX_SOURCE && BSIZE */
#else /* HAVE_STRUCT_STAT_ST_BLOCKS */
/* Some systems, like Sequents, return st_blksize of 0 on pipes. */
# define ST_BLKSIZE(statbuf) ((statbuf).st_blksize > 0 \
			       ? (statbuf).st_blksize : DEV_BSIZE)
# if defined(hpux) || defined(__hpux__) || defined(__hpux)
/* HP-UX counts st_blocks in 1024-byte units.
   This loses when mixing HP-UX and BSD filesystems with NFS.  */
#  define ST_NBLOCKSIZE 1024
# else /* !hpux */
#  if defined(_AIX) && defined(_I386)
/* AIX PS/2 counts st_blocks in 4K units.  */
#   define ST_NBLOCKSIZE (4 * 1024)
#  else /* not AIX PS/2 */
#   if defined(_CRAY)
#    define ST_NBLOCKS(statbuf) \
  (S_ISREG ((statbuf).st_mode) \
   || S_ISDIR ((statbuf).st_mode) \
   ? (statbuf).st_blocks * ST_BLKSIZE(statbuf)/ST_NBLOCKSIZE : 0)
#   endif /* _CRAY */
#  endif /* not AIX PS/2 */
# endif /* !hpux */
#endif /* HAVE_STRUCT_STAT_ST_BLOCKS */

#ifndef ST_NBLOCKS
# define ST_NBLOCKS(statbuf) \
  (S_ISREG ((statbuf).st_mode) \
   || S_ISDIR ((statbuf).st_mode) \
   ? (statbuf).st_blocks : 0)
#endif

#ifndef ST_NBLOCKSIZE
# define ST_NBLOCKSIZE 512
#endif

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


char * get_link_name (char *name, char *relname);
static void print_name_with_quoting (register char *p, FILE *stream);


/* NAME is the name to print.
   RELNAME is the path to access it from the current directory.
   STATP is the results of stat or lstat on it.
   Use CURRENT_TIME to decide whether to print yyyy or hh:mm.
   Use OUTPUT_BLOCK_SIZE to determine how to print file block counts
   and sizes.
   STREAM is the stdio stream to print on.  */

void
list_file (char *name,
	   char *relname,
	   struct stat *statp,
	   time_t current_time,
	   int output_block_size,
	   FILE *stream)
{
  char modebuf[12];
  struct tm const *when_local;
  char const *user_name;
  char const *group_name;
  char hbuf[LONGEST_HUMAN_READABLE + 1];

#if HAVE_ST_DM_MODE
  /* Cray DMF: look at the file's migrated, not real, status */
  strmode (statp->st_dm_mode, modebuf);
#else
  strmode (statp->st_mode, modebuf);
#endif

  fprintf (stream, "%6s ",
	   human_readable ((uintmax_t) statp->st_ino, hbuf,
			   human_ceiling,
			   1, 1));

  fprintf (stream, "%4s ",
	   human_readable ((uintmax_t) ST_NBLOCKS (*statp), hbuf,
			   human_ceiling,
			   ST_NBLOCKSIZE, output_block_size));


  /* modebuf includes the space between the mode and the number of links,
     as the POSIX "optional alternate access method flag".  */
  fprintf (stream, "%s%3lu ", modebuf, (unsigned long) statp->st_nlink);

  user_name = getuser (statp->st_uid);
  if (user_name)
    fprintf (stream, "%-8s ", user_name);
  else
    fprintf (stream, "%-8lu ", (unsigned long) statp->st_uid);

  group_name = getgroup (statp->st_gid);
  if (group_name)
    fprintf (stream, "%-8s ", group_name);
  else
    fprintf (stream, "%-8lu ", (unsigned long) statp->st_gid);

  if (S_ISCHR (statp->st_mode) || S_ISBLK (statp->st_mode))
#ifdef HAVE_ST_RDEV
    fprintf (stream, "%3lu, %3lu ",
	     (unsigned long) major (statp->st_rdev),
	     (unsigned long) minor (statp->st_rdev));
#else
    fprintf (stream, "         ");
#endif
  else
    fprintf (stream, "%8s ",
	     human_readable ((uintmax_t) statp->st_size, hbuf,
			     human_ceiling,
			     1,
			     output_block_size < 0 ? output_block_size : 1));

  if ((when_local = localtime (&statp->st_mtime)))
    {
      char init_bigbuf[256];
      char *buf = init_bigbuf;
      size_t bufsize = sizeof init_bigbuf;

      /* Use strftime rather than ctime, because the former can produce
	 locale-dependent names for the month (%b).

	 Output the year if the file is fairly old or in the future.
	 POSIX says the cutoff is 6 months old;
	 approximate this by 6*30 days.
	 Allow a 1 hour slop factor for what is considered "the future",
	 to allow for NFS server/client clock disagreement.  */
      char const *fmt =
	((current_time - 6 * 30 * 24 * 60 * 60 <= statp->st_mtime
	  && statp->st_mtime <= current_time + 60 * 60)
	 ? "%b %e %H:%M"
	 : "%b %e  %Y");

      while (!strftime (buf, bufsize, fmt, when_local))
	buf = (char *) alloca (bufsize *= 2);

      fprintf (stream, "%s ", buf);
    }
  else
    {
      /* The time cannot be represented as a local time;
	 print it as a huge integer number of seconds.  */
      int width = 12;

      if (statp->st_mtime < 0)
	{
	  char const *num = human_readable (- (uintmax_t) statp->st_mtime,
					    hbuf, human_ceiling, 1, 1);
	  int sign_width = width - strlen (num);
	  fprintf (stream, "%*s%s ",
		   sign_width < 0 ? 0 : sign_width, "-", num);
	}
      else
	fprintf (stream, "%*s ", width,
		 human_readable ((uintmax_t) statp->st_mtime, hbuf,
				 human_ceiling,
				 1, 1));
    }

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

static void
print_name_with_quoting (register char *p, FILE *stream)
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
get_link_name (char *name, char *relname)
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
