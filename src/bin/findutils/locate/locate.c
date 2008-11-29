/* locate -- search databases for filenames that match patterns
   Copyright (C) 1994, 1996, 1998, 1999, 2000, 2003,
                 2004, 2005, 2007 Free Software Foundation, Inc.

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
   128-255	bigram codes (the 128 most common, as determined by `updatedb')
   32-127	single character (printable) ASCII remainder

   Earlier versions of GNU locate used to use a novel two-tiered
   string search technique, which was described in Usenix ;login:, Vol
   8, No 1, February/March, 1983, p. 8.

   However, latterly code changes to provide additional functionality
   became dificult to make with the existing reading scheme, and so 
   we no longer perform the matching as efficiently as we used to (that is, 
   we no longer use the same algorithm).

   The old algorithm was:

      First, match a metacharacter-free subpattern and a partial
      pathname BACKWARDS to avoid full expansion of the pathname list.
      The time savings is 40-50% over forward matching, which cannot
      efficiently handle overlapped search patterns and compressed
      path remainders.
      
      Then, match the actual shell glob pattern (if in this form)
      against the candidate pathnames using the slower shell filename
      matching routines.


   Written by James A. Woods <jwoods@adobe.com>.
   Modified by David MacKenzie <djm@gnu.org>.  
   Additional work by James Youngman and Bas van Gompel.
*/

#include <config.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <fnmatch.h>
#include <getopt.h>
#include <xstrtol.h>

/* The presence of unistd.h is assumed by gnulib these days, so we 
 * might as well assume it too. 
 */
/* We need <unistd.h> for isatty(). */
#include <unistd.h>


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
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#else
extern int errno;
#endif

#ifdef HAVE_LOCALE_H
#include <locale.h>
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
#include "xalloc.h"
#include "error.h"
#include "human.h"
#include "dirname.h"
#include "closeout.h"
#include "nextelem.h"
#include "regex.h"
#include "quote.h"
#include "quotearg.h"
#include "printquoted.h"
#include "regextype.h"
#include "gnulib-version.h"

/* Note that this evaluates C many times.  */
#ifdef _LIBC
# define TOUPPER(Ch) toupper (Ch)
# define TOLOWER(Ch) tolower (Ch)
#else
# define TOUPPER(Ch) (islower (Ch) ? toupper (Ch) : (Ch))
# define TOLOWER(Ch) (isupper (Ch) ? tolower (Ch) : (Ch))
#endif

/* typedef enum {false, true} boolean; */

/* Warn if a database is older than this.  8 days allows for a weekly
   update that takes up to a day to perform.  */
#define WARN_NUMBER_UNITS (8)
/* Printable name of units used in WARN_SECONDS */
static const char warn_name_units[] = N_("days");
#define SECONDS_PER_UNIT (60 * 60 * 24)

#define WARN_SECONDS ((SECONDS_PER_UNIT) * (WARN_NUMBER_UNITS))

enum visit_result
  {
    VISIT_CONTINUE = 1,  /* please call the next visitor */
    VISIT_ACCEPTED = 2,  /* accepted, call no futher callbacks for this file */
    VISIT_REJECTED = 4,  /* rejected, process next file. */
    VISIT_ABORT    = 8   /* rejected, process no more files. */
  };

enum ExistenceCheckType 
  {
    ACCEPT_EITHER,		/* Corresponds to lack of -E/-e option */
    ACCEPT_EXISTING,		/* Corresponds to option -e */
    ACCEPT_NON_EXISTING		/* Corresponds to option -E */
  };

/* Check for existence of files before printing them out? */
enum ExistenceCheckType check_existence = ACCEPT_EITHER;

static int follow_symlinks = 1;

/* What to separate the results with. */
static int separator = '\n';

static struct quoting_options * quote_opts = NULL;
static bool stdout_is_a_tty;
static bool print_quoted_filename;

/* Read in a 16-bit int, high byte first (network byte order).  */

static short
get_short (FILE *fp)
{

  register short x;

  x = (signed char) fgetc (fp) << 8;
  x |= (fgetc (fp) & 0xff);
  return x;
}

const char * const metacharacters = "*?[]\\";

/* Return nonzero if S contains any shell glob characters.
 */
static int 
contains_metacharacter(const char *s)
{
  if (NULL == strpbrk(s, metacharacters))
    return 0;
  else
    return 1;
}

/* locate_read_str()
 *
 * Read bytes from FP into the buffer at offset OFFSET in (*BUF),
 * until we reach DELIMITER or end-of-file.   We reallocate the buffer 
 * as necessary, altering (*BUF) and (*SIZ) as appropriate.  No assumption 
 * is made regarding the content of the data (i.e. the implementation is 
 * 8-bit clean, the only delimiter is DELIMITER).
 *
 * Written Fri May 23 18:41:16 2003 by James Youngman, because getstr() 
 * has been removed from gnulib.  
 *
 * We call the function locate_read_str() to avoid a name clash with the curses
 * function getstr().
 */
static int
locate_read_str(char **buf, size_t *siz, FILE *fp, int delimiter, int offs)
{
  char * p = NULL;
  size_t sz = 0;
  int needed, nread;

  nread = getdelim(&p, &sz, delimiter, fp);
  if (nread >= 0)
    {
      assert(p != NULL);
      
      needed = offs + nread + 1;
      if (needed > (*siz))
	{
	  char *pnew = realloc(*buf, needed);
	  if (NULL == pnew)
	    {
	      return -1;	/* FAIL */
	    }
	  else
	    {
	      *siz = needed;
	      *buf = pnew;
	    }
	}
      memcpy((*buf)+offs, p, nread);
      free(p);
    }
  return nread;
}


static void
lc_strcpy(char *dest, const char *src)
{
  while (*src)
    {
      *dest++ = TOLOWER(*src);
      ++src;
    }
  *dest = 0;
}

struct locate_limits
{
  uintmax_t limit;
  uintmax_t items_accepted;
};
static struct locate_limits limits;


struct locate_stats
{
  uintmax_t compressed_bytes;
  uintmax_t total_filename_count;
  uintmax_t total_filename_length;
  uintmax_t whitespace_count;
  uintmax_t newline_count;
  uintmax_t highbit_filename_count;
};
static struct locate_stats statistics;


struct stringbuf
{
  char *buffer;
  size_t buffersize;
  size_t *preqlen;
};
static struct stringbuf casebuf;


struct casefolder
{
  const char *pattern;
  struct stringbuf *pbuf;
};

struct regular_expression
{
  struct re_pattern_buffer regex; /* for --regex */
};


struct process_data
{
  int c;			/* An input byte.  */
  int count; /* The length of the prefix shared with the previous database entry.  */
  int len;
  char *original_filename;	/* The current input database entry. */
  size_t pathsize;		/* Amount allocated for it.  */
  char *munged_filename;	/* path or basename(path) */
  FILE *fp;			/* The pathname database.  */
  char *dbfile;			/* Its name, or "<stdin>" */
  /* for the old database format,
     the first and second characters of the most common bigrams.  */
  char bigram1[128];
  char bigram2[128];
};


typedef int (*visitfunc)(struct process_data *procdata,
			 void *context);

struct visitor
{
  visitfunc      inspector;
  void *         context;
  struct visitor *next;
};


static struct visitor *inspectors = NULL;
static struct visitor *lastinspector = NULL;
static struct visitor *past_pat_inspector = NULL;

/* 0 or 1 pattern(s) */
static int
process_simple(struct process_data *procdata)
{
  int result = VISIT_CONTINUE;
  const struct visitor *p = inspectors;
  
  while ( ((VISIT_CONTINUE | VISIT_ACCEPTED) & result) && (NULL != p) )
    {
      result = (p->inspector)(procdata, p->context);
      p = p->next;
    }

    return result;
}

/* Accept if any pattern matches. */
static int
process_or (struct process_data *procdata)
{
  int result = VISIT_CONTINUE;
  const struct visitor *p = inspectors;
  
  while ( ((VISIT_CONTINUE | VISIT_REJECTED) & result) && (past_pat_inspector != p) )
    {
      result = (p->inspector)(procdata, p->context);
      p = p->next;
    }

  if (result == VISIT_CONTINUE)
    result = VISIT_REJECTED;
  if (result & (VISIT_ABORT | VISIT_REJECTED))
    return result;

  p = past_pat_inspector;
  result = VISIT_CONTINUE;

  while ( (VISIT_CONTINUE == result) && (NULL != p) )
    {
      result = (p->inspector)(procdata, p->context);
      p = p->next;
    }
  
  if (VISIT_CONTINUE == result)
    return VISIT_ACCEPTED;
  else
    return result;
}

/* Accept if all pattern match. */
static int
process_and (struct process_data *procdata)
{
  int result = VISIT_CONTINUE;
  const struct visitor *p = inspectors;
  
  while ( ((VISIT_CONTINUE | VISIT_ACCEPTED) & result) && (past_pat_inspector != p) )
    {
      result = (p->inspector)(procdata, p->context);
      p = p->next;
    }

  if (result == VISIT_CONTINUE)
    result = VISIT_REJECTED;
  if (result & (VISIT_ABORT | VISIT_REJECTED))
    return result;

  p = past_pat_inspector;
  result = VISIT_CONTINUE;

  while ( (VISIT_CONTINUE == result) && (NULL != p) )
    {
      result = (p->inspector)(procdata, p->context);
      p = p->next;
    }
  
  if (VISIT_CONTINUE == result)
    return VISIT_ACCEPTED;
  else
    return result;
}

typedef int (*processfunc)(struct process_data *procdata);

static processfunc mainprocessor = NULL;

static void
add_visitor(visitfunc fn, void *context)
{
  struct visitor *p = xmalloc(sizeof(struct visitor));
  p->inspector = fn;
  p->context   = context;
  p->next = NULL;
  
  if (NULL == lastinspector)
    {
      lastinspector = inspectors = p;
    }
  else
    {
      lastinspector->next = p;
      lastinspector = p;
    }
}



static int
visit_justprint_quoted(struct process_data *procdata, void *context)
{
  (void) context;
  print_quoted (stdout, quote_opts, stdout_is_a_tty,
		"%s", 
		procdata->original_filename);
  putchar(separator);
  return VISIT_CONTINUE;
}

static int
visit_justprint_unquoted(struct process_data *procdata, void *context)
{
  (void) context;
  fputs(procdata->original_filename, stdout);
  putchar(separator);
  return VISIT_CONTINUE;
}

static void
toolong (struct process_data *procdata)
{
  error (1, 0,
	 _("locate database %s contains a "
	   "filename longer than locate can handle"),
	 procdata->dbfile);
}

static void
extend (struct process_data *procdata, size_t siz1, size_t siz2)
{
  /* Figure out if the addition operation is safe before performing it. */
  if (SIZE_MAX - siz1 < siz2)
    {
      toolong (procdata);
    }
  else if (procdata->pathsize < (siz1+siz2))
    {
      procdata->pathsize = siz1+siz2;
      procdata->original_filename = x2nrealloc (procdata->original_filename,
						&procdata->pathsize,
						1);
    }
}

static int
visit_old_format(struct process_data *procdata, void *context)
{
  register size_t i;
  (void) context;

  if (EOF == procdata->c)
    return VISIT_ABORT;

  /* Get the offset in the path where this path info starts.  */
  if (procdata->c == LOCATEDB_OLD_ESCAPE)
    procdata->count += getw (procdata->fp) - LOCATEDB_OLD_OFFSET;
  else
    procdata->count += procdata->c - LOCATEDB_OLD_OFFSET;
  assert(procdata->count > 0);

  /* Overlay the old path with the remainder of the new.  Read 
   * more data until we get to the next filename.
   */
  for (i=procdata->count;
       (procdata->c = getc (procdata->fp)) > LOCATEDB_OLD_ESCAPE;)
    {
      if (EOF == procdata->c)
	break;

      if (procdata->c < 0200)
	{
	  /* An ordinary character. */	  
	  extend (procdata, i, 1u);
	  procdata->original_filename[i++] = procdata->c;
	}
      else
	{
	  /* Bigram markers have the high bit set. */
	  extend (procdata, i, 2u);
	  procdata->c &= 0177;
	  procdata->original_filename[i++] = procdata->bigram1[procdata->c];
	  procdata->original_filename[i++] = procdata->bigram2[procdata->c];
	}
    }

  /* Consider the case where we executed the loop body zero times; we
   * still need space for the terminating null byte. 
   */
  extend (procdata, i, 1u);
  procdata->original_filename[i] = 0;

  procdata->munged_filename = procdata->original_filename;
  
  return VISIT_CONTINUE;
}


static int
visit_locate02_format(struct process_data *procdata, void *context)
{
  register char *s;
  int nread;
  (void) context;

  if (procdata->c == LOCATEDB_ESCAPE)
    procdata->count += (short)get_short (procdata->fp);
  else if (procdata->c > 127)
    procdata->count += procdata->c - 256;
  else
    procdata->count += procdata->c;

  if (procdata->count > procdata->len || procdata->count < 0)
    {
      /* This should not happen generally , but since we're
       * reading in data which is outside our control, we
       * cannot prevent it.
       */
      error(1, 0, _("locate database `%s' is corrupt or invalid"), procdata->dbfile);
    }
 
  /* Overlay the old path with the remainder of the new.  */
  nread = locate_read_str (&procdata->original_filename, &procdata->pathsize,
			   procdata->fp, 0, procdata->count);
  if (nread < 0)
    return VISIT_ABORT;
  procdata->c = getc (procdata->fp);
  procdata->len = procdata->count + nread;
  s = procdata->original_filename + procdata->len - 1; /* Move to the last char in path.  */
  assert (s[0] != '\0');
  assert (s[1] == '\0'); /* Our terminator.  */
  assert (s[2] == '\0'); /* Added by locate_read_str.  */

  procdata->munged_filename = procdata->original_filename;
  
  return VISIT_CONTINUE;
}

static int
visit_basename(struct process_data *procdata, void *context)
{
  (void) context;
  procdata->munged_filename = last_component (procdata->original_filename);

  return VISIT_CONTINUE;
}


static int
visit_casefold(struct process_data *procdata, void *context)
{
  struct stringbuf *b = context;

  if (*b->preqlen+1 > b->buffersize)
    {
      b->buffer = xrealloc(b->buffer, *b->preqlen+1); /* XXX: consider using extendbuf(). */
      b->buffersize = *b->preqlen+1;
    }
  lc_strcpy(b->buffer, procdata->munged_filename);

  return VISIT_CONTINUE;
}
  
/* visit_existing_follow implements -L -e */
static int
visit_existing_follow(struct process_data *procdata, void *context)
{
  struct stat st;
  (void) context;

  /* munged_filename has been converted in some way (to lower case,
   * or is just the base name of the file), and original_filename has not.  
   * Hence only original_filename is still actually the name of the file 
   * whose existence we would need to check.
   */
  if (stat(procdata->original_filename, &st) != 0)
    {
      return VISIT_REJECTED;
    }
  else
    {
      return VISIT_CONTINUE;
    }
}

/* visit_non_existing_follow implements -L -E */
static int
visit_non_existing_follow(struct process_data *procdata, void *context)
{
  struct stat st;
  (void) context;

  /* munged_filename has been converted in some way (to lower case,
   * or is just the base name of the file), and original_filename has not.  
   * Hence only original_filename is still actually the name of the file 
   * whose existence we would need to check.
   */
  if (stat(procdata->original_filename, &st) == 0)
    {
      return VISIT_REJECTED;
    }
  else
    {
      return VISIT_CONTINUE;
    }
}

/* visit_existing_nofollow implements -P -e */
static int
visit_existing_nofollow(struct process_data *procdata, void *context)
{
  struct stat st;
  (void) context;

  /* munged_filename has been converted in some way (to lower case,
   * or is just the base name of the file), and original_filename has not.  
   * Hence only original_filename is still actually the name of the file 
   * whose existence we would need to check.
   */
  if (lstat(procdata->original_filename, &st) != 0)
    {
      return VISIT_REJECTED;
    }
  else
    {
      return VISIT_CONTINUE;
    }
}

/* visit_non_existing_nofollow implements -P -E */
static int
visit_non_existing_nofollow(struct process_data *procdata, void *context)
{
  struct stat st;
  (void) context;

  /* munged_filename has been converted in some way (to lower case,
   * or is just the base name of the file), and original_filename has not.  
   * Hence only original_filename is still actually the name of the file 
   * whose existence we would need to check.
   */
  if (lstat(procdata->original_filename, &st) == 0)
    {
      return VISIT_REJECTED;
    }
  else
    {
      return VISIT_CONTINUE;
    }
}

static int
visit_substring_match_nocasefold(struct process_data *procdata, void *context)
{
  const char *pattern = context;

  if (NULL != strstr(procdata->munged_filename, pattern))
    return VISIT_ACCEPTED;
  else
    return VISIT_REJECTED;
}

static int
visit_substring_match_casefold(struct process_data *procdata, void *context)
{
  const struct casefolder * p = context;
  const struct stringbuf * b = p->pbuf;
  (void) procdata;

  if (NULL != strstr(b->buffer, p->pattern))
    return VISIT_ACCEPTED;
  else
    return VISIT_REJECTED;
}


static int
visit_globmatch_nofold(struct process_data *procdata, void *context)
{
  const char *glob = context;
  if (fnmatch(glob, procdata->munged_filename, 0) != 0)
    return VISIT_REJECTED;
  else
    return VISIT_ACCEPTED;
}


static int
visit_globmatch_casefold(struct process_data *procdata, void *context)
{
  const char *glob = context;
  if (fnmatch(glob, procdata->munged_filename, FNM_CASEFOLD) != 0)
    return VISIT_REJECTED;
  else
    return VISIT_ACCEPTED;
}


static int
visit_regex(struct process_data *procdata, void *context)
{
  struct regular_expression *p = context;
  const size_t len = strlen(procdata->munged_filename);

  int rv = re_search (&p->regex, procdata->munged_filename,
		      len, 0, len, 
		      (struct re_registers *) NULL);
  if (rv < 0)
    {
      return VISIT_REJECTED;	/* no match (-1), or internal error (-2) */
    }
  else 
    {
      return VISIT_ACCEPTED;	/* match */
    }
}


static int
visit_stats(struct process_data *procdata, void *context)
{
  struct locate_stats *p = context;
  size_t len = strlen(procdata->original_filename);
  const char *s;
  int highbit, whitespace, newline;
  
  ++(p->total_filename_count);
  p->total_filename_length += len;
  
  highbit = whitespace = newline = 0;
  for (s=procdata->original_filename; *s; ++s)
    {
      if ( (int)(*s) & 128 )
	highbit = 1;
      if ('\n' == *s)
	{
	  newline = whitespace = 1;
	}
      else if (isspace((unsigned char)*s))
	{
	  whitespace = 1;
	}
    }

  if (highbit)
    ++(p->highbit_filename_count);
  if (whitespace)
    ++(p->whitespace_count);
  if (newline)
    ++(p->newline_count);

  return VISIT_CONTINUE;
}


static int
visit_limit(struct process_data *procdata, void *context)
{
  struct locate_limits *p = context;

  (void) procdata;

  if (++p->items_accepted >= p->limit)
    return VISIT_ABORT;
  else
    return VISIT_CONTINUE;
}

static int
visit_count(struct process_data *procdata, void *context)
{
  struct locate_limits *p = context;

  (void) procdata;

  ++p->items_accepted;
  return VISIT_CONTINUE;
}

/* Emit the statistics.
 */
static void
print_stats(int argc, size_t database_file_size)
{
  char hbuf[LONGEST_HUMAN_READABLE + 1];
  
  printf(_("Locate database size: %s bytes\n"),
	 human_readable ((uintmax_t) database_file_size,
			 hbuf, human_ceiling, 1, 1));
  
  printf(_("Filenames: %s "),
	 human_readable (statistics.total_filename_count,
			 hbuf, human_ceiling, 1, 1));
  printf(_("with a cumulative length of %s bytes"),
	 human_readable (statistics.total_filename_length,
			 hbuf, human_ceiling, 1, 1));
  
  printf(_("\n\tof which %s contain whitespace, "),
	 human_readable (statistics.whitespace_count,
			 hbuf, human_ceiling, 1, 1));
  printf(_("\n\t%s contain newline characters, "),
	 human_readable (statistics.newline_count,
			 hbuf, human_ceiling, 1, 1));
  printf(_("\n\tand %s contain characters with the high bit set.\n"),
	 human_readable (statistics.highbit_filename_count,
			 hbuf, human_ceiling, 1, 1));
  
  if (!argc)
    printf(_("Compression ratio %4.2f%%\n"),
	   100.0 * ((double)statistics.total_filename_length
		    - (double) database_file_size)
	   / (double) statistics.total_filename_length);
  printf("\n");
}


/* Print or count the entries in DBFILE that match shell globbing patterns in 
   ARGV. Return the number of entries matched. */

static unsigned long
locate (int argc,
	char **argv,
	char *dbfile,
	int ignore_case,
	int enable_print,
	int basename_only,
	int use_limit,
	struct locate_limits *plimit,
	int stats,
	int op_and,
	int regex,
	int regex_options)
{
  char *pathpart;		/* A pattern to consider. */
  int argn;			/* Index to current pattern in argv. */
  int need_fold;	/* Set when folding and any pattern is non-glob. */
  int nread;		     /* number of bytes read from an entry. */
  struct process_data procdata;	/* Storage for data shared with visitors. */
  
  int old_format = 0; /* true if reading a bigram-encoded database.  */
  static bool did_stdin = false; /* Set to prevent rereading stdin. */
  struct visitor* pvis; /* temp for determining past_pat_inspector. */
  
  /* To check the age of the database.  */
  struct stat st;
  time_t now;


  if (ignore_case)
    regex_options |= RE_ICASE;
  
  procdata.len = procdata.count = 0;
  if (!strcmp (dbfile, "-"))
    {
      if (did_stdin)
	{
	  error (0, 0, _("warning: the locate database can only be read from stdin once."));
	  return 0;
	}
      
	    
      procdata.dbfile = "<stdin>";
      procdata.fp = stdin;
      did_stdin = true;
    }
  else
    {
      if (stat (dbfile, &st) || (procdata.fp = fopen (dbfile, "r")) == NULL)
	{
	  error (0, errno, "%s", dbfile);
	  return 0;
	}
      time(&now);
      if (now - st.st_mtime > WARN_SECONDS)
	{
	  /* For example:
	     warning: database `fred' is more than 8 days old */
	  error (0, 0, _("warning: database `%s' is more than %d %s old"),
		 dbfile, WARN_NUMBER_UNITS, _(warn_name_units));
	}
      procdata.dbfile = dbfile;
    }

  procdata.pathsize = 1026;	/* Increased as necessary by locate_read_str.  */
  procdata.original_filename = xmalloc (procdata.pathsize);

  nread = fread (procdata.original_filename, 1, sizeof (LOCATEDB_MAGIC),
		 procdata.fp);
  if (nread != sizeof (LOCATEDB_MAGIC)
      || memcmp (procdata.original_filename, LOCATEDB_MAGIC,
		 sizeof (LOCATEDB_MAGIC)))
    {
      int i;
      /* Read the list of the most common bigrams in the database.  */
      nread = fread (procdata.original_filename + sizeof (LOCATEDB_MAGIC), 1,
	  256 - sizeof (LOCATEDB_MAGIC), procdata.fp);
      for (i = 0; i < 128; i++)
	{
	  procdata.bigram1[i] = procdata.original_filename[i << 1];
	  procdata.bigram2[i] = procdata.original_filename[(i << 1) + 1];
	}
      old_format = 1;
    }

  /* Set up the inspection regime */
  inspectors = NULL;
  lastinspector = NULL;
  past_pat_inspector = NULL;

  if (old_format)
    add_visitor(visit_old_format, NULL);
  else
    add_visitor(visit_locate02_format, NULL);

  if (basename_only)
    add_visitor(visit_basename, NULL);
  
  /* See if we need fold. */
  if (ignore_case && !regex)
    for ( argn = 0; argn < argc; argn++ )
      {
        pathpart = argv[argn];
        if (!contains_metacharacter(pathpart))
	  {
	    need_fold = 1;
	    break;
	  }
      }

  if (need_fold)
    {
      add_visitor(visit_casefold, &casebuf);
      casebuf.preqlen = &procdata.pathsize;
    }
  
  /* Add an inspector for each pattern we're looking for. */
  for ( argn = 0; argn < argc; argn++ )
    {
      pathpart = argv[argn];
      if (regex)
	{
	  struct regular_expression *p = xmalloc(sizeof(*p));
	  const char *error_message = NULL;
	  
	  memset (&p->regex, 0, sizeof (p->regex));
	  
	  re_set_syntax(regex_options);
	  p->regex.allocated = 100;
	  p->regex.buffer = (unsigned char *) xmalloc (p->regex.allocated);
	  p->regex.fastmap = NULL;
	  p->regex.syntax = regex_options;
	  p->regex.translate = NULL;

	  error_message = re_compile_pattern (pathpart, strlen (pathpart),
					      &p->regex);
	  if (error_message)
	    {
	      error (1, 0, "%s", error_message);
	    }
	  else 
	    {
	      add_visitor(visit_regex, p);
	    }
	}
      else if (contains_metacharacter(pathpart))
	{
	  if (ignore_case)
	    add_visitor(visit_globmatch_casefold, pathpart);
	  else
	    add_visitor(visit_globmatch_nofold, pathpart);
	}
      else
	{
	  /* No glob characters used.  Hence we match on 
	   * _any part_ of the filename, not just the 
	   * basename.  This seems odd to me, but it is the 
	   * traditional behaviour.
	   * James Youngman <jay@gnu.org> 
	   */
	  if (ignore_case)
	    {
	      struct casefolder * cf = xmalloc(sizeof(*cf));
	      cf->pattern = pathpart;
	      cf->pbuf = &casebuf;
	      add_visitor(visit_substring_match_casefold, cf);
	      /* If we ignore case, convert it to lower now so we don't have to
	       * do it every time
	       */
	      lc_strcpy(pathpart, pathpart);
	    }
	  else
	    {
	      add_visitor(visit_substring_match_nocasefold, pathpart);
	    }
	}
    }

  pvis = lastinspector;

  /* We add visit_existing_*() as late as possible to reduce the
   * number of stat() calls.
   */
  switch (check_existence)
    {
      case ACCEPT_EXISTING:
	if (follow_symlinks)	/* -L, default */
	  add_visitor(visit_existing_follow, NULL);
	else			/* -P */
	  add_visitor(visit_existing_nofollow, NULL);
	break;
	  
      case ACCEPT_NON_EXISTING:
	if (follow_symlinks)	/* -L, default */
	  add_visitor(visit_non_existing_follow, NULL);
	else			/* -P */
	  add_visitor(visit_non_existing_nofollow, NULL);
	break;

      case ACCEPT_EITHER:	/* Default, neither -E nor -e */
	/* do nothing; no extra processing. */
	break;
    }
      
  if (stats)
    add_visitor(visit_stats, &statistics);

  if (enable_print)
    {
      if (print_quoted_filename)
	add_visitor(visit_justprint_quoted,   NULL);
      else
	add_visitor(visit_justprint_unquoted, NULL);
    }


  if (use_limit)
    add_visitor(visit_limit, plimit);
  else
    add_visitor(visit_count, plimit);


  if (argc > 1)
    {
      past_pat_inspector = pvis->next;
      if (op_and)
        mainprocessor = process_and;
      else
        mainprocessor = process_or;
    }
  else
    mainprocessor = process_simple;

  if (stats)
    {
	printf(_("Database %s is in the %s format.\n"),
	       procdata.dbfile,
	       old_format ? _("old") : "LOCATE02");
    }


  procdata.c = getc (procdata.fp);
  /* If we are searching for filename patterns, the inspector list 
   * will contain an entry for each pattern for which we are searching.
   */
  while ( (procdata.c != EOF) &&
          (VISIT_ABORT != (mainprocessor)(&procdata)) )
    {
      /* Do nothing; all the work is done in the visitor functions. */
    }
  
  if (stats)
    {
      print_stats(argc, st.st_size);
    }
  
  if (ferror (procdata.fp))
    {
      error (0, errno, "%s", procdata.dbfile);
      return 0;
    }
  if (procdata.fp != stdin && fclose (procdata.fp) == EOF)
    {
      error (0, errno, "%s", dbfile);
      return 0;
    }

  return plimit->items_accepted;
}




extern char *version_string;

/* The name this program was run with. */
char *program_name;

static void
usage (FILE *stream)
{
  fprintf (stream, _("\
Usage: %s [-d path | --database=path] [-e | -E | --[non-]existing]\n\
      [-i | --ignore-case] [-w | --wholename] [-b | --basename] \n\
      [--limit=N | -l N] [-S | --statistics] [-0 | --null] [-c | --count]\n\
      [-P | -H | --nofollow] [-L | --follow] [-m | --mmap ] [ -s | --stdio ]\n\
      [-A | --all] [-p | --print] [-r | --regex ] [--regextype=TYPE]\n\
      [--version] [--help]\n\
      pattern...\n"),
	   program_name);
  fputs (_("\nReport bugs to <bug-findutils@gnu.org>.\n"), stream);
}
enum
  {
    REGEXTYPE_OPTION = CHAR_MAX + 1
  };


static struct option const longopts[] =
{
  {"database", required_argument, NULL, 'd'},
  {"existing", no_argument, NULL, 'e'},
  {"non-existing", no_argument, NULL, 'E'},
  {"ignore-case", no_argument, NULL, 'i'},
  {"all", no_argument, NULL, 'A'},
  {"help", no_argument, NULL, 'h'},
  {"version", no_argument, NULL, 'v'},
  {"null", no_argument, NULL, '0'},
  {"count", no_argument, NULL, 'c'},
  {"wholename", no_argument, NULL, 'w'},
  {"wholepath", no_argument, NULL, 'w'}, /* Synonym. */
  {"basename", no_argument, NULL, 'b'},
  {"print", no_argument, NULL, 'p'},
  {"stdio", no_argument, NULL, 's'},
  {"mmap",  no_argument, NULL, 'm'},
  {"limit",  required_argument, NULL, 'l'},
  {"regex",  no_argument, NULL, 'r'},
  {"regextype",  required_argument, NULL, REGEXTYPE_OPTION},
  {"statistics",  no_argument, NULL, 'S'},
  {"follow",      no_argument, NULL, 'L'},
  {"nofollow",    no_argument, NULL, 'P'},
  {NULL, no_argument, NULL, 0}
};

int
main (int argc, char **argv)
{
  char *dbpath;
  unsigned long int found = 0uL;
  int ignore_case = 0;
  int print = 0;
  int just_count = 0;
  int basename_only = 0;
  int use_limit = 0;
  int regex = 0;
  int regex_options = RE_SYNTAX_EMACS;
  int stats = 0;
  int op_and = 0;
  char *e;
  
  program_name = argv[0];

#ifdef HAVE_SETLOCALE
  setlocale (LC_ALL, "");
#endif
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
  atexit (close_stdout);

  limits.limit = 0;
  limits.items_accepted = 0;

  quote_opts = clone_quoting_options (NULL);
  print_quoted_filename = true;
  
  dbpath = getenv ("LOCATE_PATH");
  if (dbpath == NULL)
    dbpath = LOCATE_DB;

  check_existence = ACCEPT_EITHER;

  for (;;)
    {
      int opti = -1;
      int optc = getopt_long (argc, argv, "Abcd:eEil:prsm0SwHPL", longopts,
			      &opti);
      if (optc == -1)
	break;

      switch (optc)
	{
	case '0':
	  separator = 0;
	  print_quoted_filename = false; /* print filename 'raw'. */
	  break;

	case 'A':
	  op_and = 1;
	  break;

	case 'b':
	  basename_only = 1;
	  break;

	case 'c':
	  just_count = 1;
	  break;

	case 'd':
	  dbpath = optarg;
	  break;

	case 'e':
	  check_existence = ACCEPT_EXISTING;
	  break;

	case 'E':
	  check_existence = ACCEPT_NON_EXISTING;
	  break;

	case 'i':
	  ignore_case = 1;
	  break;

	case 'h':
	  usage (stdout);
	  return 0;

	case 'p':
	  print = 1;
	  break;

	case 'v':
	  printf (_("GNU locate version %s\n"), version_string);
	  printf (_("Built using GNU gnulib version %s\n"), gnulib_version);
	  return 0;

	case 'w':
	  basename_only = 0;
	  break;

	case 'r':
	  regex = 1;
	  break;

	case REGEXTYPE_OPTION:
	  regex_options = get_regex_type (optarg);
	  break;

	case 'S':
	  stats = 1;
	  break;

	case 'L':
	  follow_symlinks = 1;
	  break;

	  /* In find, -P and -H differ in the way they handle paths
	   * given on the command line.  This is not relevant for
	   * locate, but the -H option is supported because it is
	   * probably more intuitive to do so.
	   */
	case 'P':
	case 'H':
	  follow_symlinks = 0;
	  break;

	case 'l':
	  {
	    char *end = optarg;
	    strtol_error err = xstrtoumax (optarg, &end, 10, &limits.limit,
					   NULL);
	    if (LONGINT_OK != err)
	      xstrtol_fatal (err, opti, optc, longopts, optarg);
	    use_limit = 1;
	  }
	  break;

	case 's':			/* use stdio */
	case 'm':			/* use mmap  */
	  /* These options are implemented simply for
	   * compatibility with FreeBSD
	   */
	  break;

	default:
	  usage (stderr);
	  return 1;
	}
    }

  if (!just_count && !stats)
    print = 1;

  if (stats)
    {
      if (optind == argc)
	use_limit = 0;
    }
  else
    {
      if (!just_count && optind == argc)
	{
	  usage (stderr);
	  return 1;
	}
    }
  

  if (1 == isatty(STDOUT_FILENO))
    stdout_is_a_tty = true;
  else
    stdout_is_a_tty = false;

  next_element (dbpath, 0);	/* Initialize.  */

  /* Bail out early if limit already reached. */
  while ((e = next_element ((char *) NULL, 0)) != NULL  &&
	 (!use_limit || limits.limit > limits.items_accepted))
    {
      statistics.compressed_bytes = 
      statistics.total_filename_count = 
      statistics.total_filename_length = 
      statistics.whitespace_count = 
      statistics.newline_count = 
      statistics.highbit_filename_count = 0u;

      if (0 == strlen(e) || 0 == strcmp(e, "."))
	{
	  /* Use the default database name instead (note: we
	   * don't use 'dbpath' since that might itself contain a 
	   * colon-separated list.
	   */
	  e = LOCATE_DB;
	}
	  
      found = locate (argc - optind, &argv[optind], e, ignore_case, print, basename_only, use_limit, &limits, stats, op_and, regex, regex_options);
    }
  
  if (just_count)
    {
      printf("%ld\n", found);
    }
  
  if (found || (use_limit && (limits.limit==0)) || stats )
    return 0;
  else
    return 1;
}
