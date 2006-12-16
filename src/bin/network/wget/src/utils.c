/* Various utility functions.
   Copyright (C) 2005 Free Software Foundation, Inc.

This file is part of GNU Wget.

GNU Wget is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GNU Wget is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wget; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

In addition, as a special exception, the Free Software Foundation
gives permission to link the code of its release of Wget with the
OpenSSL project's "OpenSSL" library (or with modified versions of it
that use the same license as the "OpenSSL" library), and distribute
the linked executables.  You must obey the GNU General Public License
in all respects for all of the code used other than "OpenSSL".  If you
modify this file, you may extend this exception to your version of the
file, but you are not obligated to do so.  If you do not wish to do
so, delete this exception statement from your version.  */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
# include <string.h>
#else  /* not HAVE_STRING_H */
# include <strings.h>
#endif /* not HAVE_STRING_H */
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_MMAP
# include <sys/mman.h>
#endif
#ifdef HAVE_PWD_H
# include <pwd.h>
#endif
#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif
#ifdef HAVE_UTIME_H
# include <utime.h>
#endif
#ifdef HAVE_SYS_UTIME_H
# include <sys/utime.h>
#endif
#include <errno.h>
#ifdef NeXT
# include <libc.h>		/* for access() */
#endif
#include <fcntl.h>
#include <assert.h>
#ifdef WGET_USE_STDARG
# include <stdarg.h>
#else
# include <varargs.h>
#endif
#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif

/* For TIOCGWINSZ and friends: */
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_TERMIOS_H
# include <termios.h>
#endif

/* Needed for run_with_timeout. */
#undef USE_SIGNAL_TIMEOUT
#ifdef HAVE_SIGNAL_H
# include <signal.h>
#endif
#ifdef HAVE_SETJMP_H
# include <setjmp.h>
#endif

#ifndef HAVE_SIGSETJMP
/* If sigsetjmp is a macro, configure won't pick it up. */
# ifdef sigsetjmp
#  define HAVE_SIGSETJMP
# endif
#endif

#ifdef HAVE_SIGNAL
# ifdef HAVE_SIGSETJMP
#  define USE_SIGNAL_TIMEOUT
# endif
# ifdef HAVE_SIGBLOCK
#  define USE_SIGNAL_TIMEOUT
# endif
#endif

#include "wget.h"
#include "utils.h"
#include "hash.h"

#ifndef errno
extern int errno;
#endif

/* Utility function: like xstrdup(), but also lowercases S.  */

char *
xstrdup_lower (const char *s)
{
  char *copy = xstrdup (s);
  char *p = copy;
  for (; *p; p++)
    *p = TOLOWER (*p);
  return copy;
}

/* Copy the string formed by two pointers (one on the beginning, other
   on the char after the last char) to a new, malloc-ed location.
   0-terminate it.  */
char *
strdupdelim (const char *beg, const char *end)
{
  char *res = (char *)xmalloc (end - beg + 1);
  memcpy (res, beg, end - beg);
  res[end - beg] = '\0';
  return res;
}

/* Parse a string containing comma-separated elements, and return a
   vector of char pointers with the elements.  Spaces following the
   commas are ignored.  */
char **
sepstring (const char *s)
{
  char **res;
  const char *p;
  int i = 0;

  if (!s || !*s)
    return NULL;
  res = NULL;
  p = s;
  while (*s)
    {
      if (*s == ',')
	{
	  res = (char **)xrealloc (res, (i + 2) * sizeof (char *));
	  res[i] = strdupdelim (p, s);
	  res[++i] = NULL;
	  ++s;
	  /* Skip the blanks following the ','.  */
	  while (ISSPACE (*s))
	    ++s;
	  p = s;
	}
      else
	++s;
    }
  res = (char **)xrealloc (res, (i + 2) * sizeof (char *));
  res[i] = strdupdelim (p, s);
  res[i + 1] = NULL;
  return res;
}

#ifdef WGET_USE_STDARG
# define VA_START(args, arg1) va_start (args, arg1)
#else
# define VA_START(args, ignored) va_start (args)
#endif

/* Like sprintf, but allocates a string of sufficient size with malloc
   and returns it.  GNU libc has a similar function named asprintf,
   which requires the pointer to the string to be passed.  */

char *
aprintf (const char *fmt, ...)
{
  /* This function is implemented using vsnprintf, which we provide
     for the systems that don't have it.  Therefore, it should be 100%
     portable.  */

  int size = 32;
  char *str = xmalloc (size);

  while (1)
    {
      int n;
      va_list args;

      /* See log_vprintf_internal for explanation why it's OK to rely
	 on the return value of vsnprintf.  */

      VA_START (args, fmt);
      n = vsnprintf (str, size, fmt, args);
      va_end (args);

      /* If the printing worked, return the string. */
      if (n > -1 && n < size)
	return str;

      /* Else try again with a larger buffer. */
      if (n > -1)		/* C99 */
	size = n + 1;		/* precisely what is needed */
      else
	size <<= 1;		/* twice the old size */
      str = xrealloc (str, size);
    }
}

/* Concatenate the NULL-terminated list of string arguments into
   freshly allocated space.  */

char *
concat_strings (const char *str0, ...)
{
  va_list args;
  int saved_lengths[5];		/* inspired by Apache's apr_pstrcat */
  char *ret, *p;

  const char *next_str;
  int total_length = 0;
  int argcount;

  /* Calculate the length of and allocate the resulting string. */

  argcount = 0;
  VA_START (args, str0);
  for (next_str = str0; next_str != NULL; next_str = va_arg (args, char *))
    {
      int len = strlen (next_str);
      if (argcount < countof (saved_lengths))
	saved_lengths[argcount++] = len;
      total_length += len;
    }
  va_end (args);
  p = ret = xmalloc (total_length + 1);

  /* Copy the strings into the allocated space. */

  argcount = 0;
  VA_START (args, str0);
  for (next_str = str0; next_str != NULL; next_str = va_arg (args, char *))
    {
      int len;
      if (argcount < countof (saved_lengths))
	len = saved_lengths[argcount++];
      else
	len = strlen (next_str);
      memcpy (p, next_str, len);
      p += len;
    }
  va_end (args);
  *p = '\0';

  return ret;
}

/* Return pointer to a static char[] buffer in which zero-terminated
   string-representation of TM (in form hh:mm:ss) is printed.

   If TM is NULL, the current time will be used.  */

char *
time_str (time_t *tm)
{
  static char output[15];
  struct tm *ptm;
  time_t secs = tm ? *tm : time (NULL);

  if (secs == -1)
    {
      /* In case of error, return the empty string.  Maybe we should
	 just abort if this happens?  */
      *output = '\0';
      return output;
    }
  ptm = localtime (&secs);
  sprintf (output, "%02d:%02d:%02d", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
  return output;
}

/* Like the above, but include the date: YYYY-MM-DD hh:mm:ss.  */

char *
datetime_str (time_t *tm)
{
  static char output[20];	/* "YYYY-MM-DD hh:mm:ss" + \0 */
  struct tm *ptm;
  time_t secs = tm ? *tm : time (NULL);

  if (secs == -1)
    {
      /* In case of error, return the empty string.  Maybe we should
	 just abort if this happens?  */
      *output = '\0';
      return output;
    }
  ptm = localtime (&secs);
  sprintf (output, "%04d-%02d-%02d %02d:%02d:%02d",
	   ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
	   ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
  return output;
}

/* The Windows versions of the following two functions are defined in
   mswindows.c.  */

#ifndef WINDOWS
void
fork_to_background (void)
{
  pid_t pid;
  /* Whether we arrange our own version of opt.lfilename here.  */
  int logfile_changed = 0;

  if (!opt.lfilename)
    {
      /* We must create the file immediately to avoid either a race
	 condition (which arises from using unique_name and failing to
	 use fopen_excl) or lying to the user about the log file name
	 (which arises from using unique_name, printing the name, and
	 using fopen_excl later on.)  */
      FILE *new_log_fp = unique_create (DEFAULT_LOGFILE, 0, &opt.lfilename);
      if (new_log_fp)
	{
	  logfile_changed = 1;
	  fclose (new_log_fp);
	}
    }
  pid = fork ();
  if (pid < 0)
    {
      /* parent, error */
      perror ("fork");
      exit (1);
    }
  else if (pid != 0)
    {
      /* parent, no error */
      printf (_("Continuing in background, pid %d.\n"), (int)pid);
      if (logfile_changed)
	printf (_("Output will be written to `%s'.\n"), opt.lfilename);
      exit (0);			/* #### should we use _exit()? */
    }

  /* child: give up the privileges and keep running. */
  setsid ();
  freopen ("/dev/null", "r", stdin);
  freopen ("/dev/null", "w", stdout);
  freopen ("/dev/null", "w", stderr);
}
#endif /* not WINDOWS */

/* "Touch" FILE, i.e. make its mtime ("modified time") equal the time
   specified with TM.  The atime ("access time") is set to the current
   time.  */

void
touch (const char *file, time_t tm)
{
#ifdef HAVE_STRUCT_UTIMBUF
  struct utimbuf times;
#else
  struct {
    time_t actime;
    time_t modtime;
  } times;
#endif
  times.modtime = tm;
  times.actime = time (NULL);
  if (utime (file, &times) == -1)
    logprintf (LOG_NOTQUIET, "utime(%s): %s\n", file, strerror (errno));
}

/* Checks if FILE is a symbolic link, and removes it if it is.  Does
   nothing under MS-Windows.  */
int
remove_link (const char *file)
{
  int err = 0;
  struct_stat st;

  if (lstat (file, &st) == 0 && S_ISLNK (st.st_mode))
    {
      DEBUGP (("Unlinking %s (symlink).\n", file));
      err = unlink (file);
      if (err != 0)
	logprintf (LOG_VERBOSE, _("Failed to unlink symlink `%s': %s\n"),
		   file, strerror (errno));
    }
  return err;
}

/* Does FILENAME exist?  This is quite a lousy implementation, since
   it supplies no error codes -- only a yes-or-no answer.  Thus it
   will return that a file does not exist if, e.g., the directory is
   unreadable.  I don't mind it too much currently, though.  The
   proper way should, of course, be to have a third, error state,
   other than true/false, but that would introduce uncalled-for
   additional complexity to the callers.  */
int
file_exists_p (const char *filename)
{
#ifdef HAVE_ACCESS
  return access (filename, F_OK) >= 0;
#else
  struct_stat buf;
  return stat (filename, &buf) >= 0;
#endif
}

/* Returns 0 if PATH is a directory, 1 otherwise (any kind of file).
   Returns 0 on error.  */
int
file_non_directory_p (const char *path)
{
  struct_stat buf;
  /* Use lstat() rather than stat() so that symbolic links pointing to
     directories can be identified correctly.  */
  if (lstat (path, &buf) != 0)
    return 0;
  return S_ISDIR (buf.st_mode) ? 0 : 1;
}

/* Return the size of file named by FILENAME, or -1 if it cannot be
   opened or seeked into. */
wgint
file_size (const char *filename)
{
#if defined(HAVE_FSEEKO) && defined(HAVE_FTELLO)
  wgint size;
  /* We use fseek rather than stat to determine the file size because
     that way we can also verify that the file is readable without
     explicitly checking for permissions.  Inspired by the POST patch
     by Arnaud Wylie.  */
  FILE *fp = fopen (filename, "rb");
  if (!fp)
    return -1;
  fseeko (fp, 0, SEEK_END);
  size = ftello (fp);
  fclose (fp);
  return size;
#else
  struct_stat st;
  if (stat (filename, &st) < 0)
    return -1;
  return st.st_size;
#endif
}

/* stat file names named PREFIX.1, PREFIX.2, etc., until one that
   doesn't exist is found.  Return a freshly allocated copy of the
   unused file name.  */

static char *
unique_name_1 (const char *prefix)
{
  int count = 1;
  int plen = strlen (prefix);
  char *template = (char *)alloca (plen + 1 + 24);
  char *template_tail = template + plen;

  memcpy (template, prefix, plen);
  *template_tail++ = '.';

  do
    number_to_string (template_tail, count++);
  while (file_exists_p (template));

  return xstrdup (template);
}

/* Return a unique file name, based on FILE.

   More precisely, if FILE doesn't exist, it is returned unmodified.
   If not, FILE.1 is tried, then FILE.2, etc.  The first FILE.<number>
   file name that doesn't exist is returned.

   The resulting file is not created, only verified that it didn't
   exist at the point in time when the function was called.
   Therefore, where security matters, don't rely that the file created
   by this function exists until you open it with O_EXCL or
   equivalent.

   If ALLOW_PASSTHROUGH is 0, it always returns a freshly allocated
   string.  Otherwise, it may return FILE if the file doesn't exist
   (and therefore doesn't need changing).  */

char *
unique_name (const char *file, int allow_passthrough)
{
  /* If the FILE itself doesn't exist, return it without
     modification. */
  if (!file_exists_p (file))
    return allow_passthrough ? (char *)file : xstrdup (file);

  /* Otherwise, find a numeric suffix that results in unused file name
     and return it.  */
  return unique_name_1 (file);
}

/* Create a file based on NAME, except without overwriting an existing
   file with that name.  Providing O_EXCL is correctly implemented,
   this function does not have the race condition associated with
   opening the file returned by unique_name.  */

FILE *
unique_create (const char *name, int binary, char **opened_name)
{
  /* unique file name, based on NAME */
  char *uname = unique_name (name, 0);
  FILE *fp;
  while ((fp = fopen_excl (uname, binary)) == NULL && errno == EEXIST)
    {
      xfree (uname);
      uname = unique_name (name, 0);
    }
  if (opened_name && fp != NULL)
    {
      if (fp)
	*opened_name = uname;
      else
	{
	  *opened_name = NULL;
	  xfree (uname);
	}
    }
  else
    xfree (uname);
  return fp;
}

/* Open the file for writing, with the addition that the file is
   opened "exclusively".  This means that, if the file already exists,
   this function will *fail* and errno will be set to EEXIST.  If
   BINARY is set, the file will be opened in binary mode, equivalent
   to fopen's "wb".

   If opening the file fails for any reason, including the file having
   previously existed, this function returns NULL and sets errno
   appropriately.  */
   
FILE *
fopen_excl (const char *fname, int binary)
{
  int fd;
#ifdef O_EXCL
  int flags = O_WRONLY | O_CREAT | O_EXCL;
# ifdef O_BINARY
  if (binary)
    flags |= O_BINARY;
# endif
  fd = open (fname, flags, 0666);
  if (fd < 0)
    return NULL;
  return fdopen (fd, binary ? "wb" : "w");
#else  /* not O_EXCL */
  /* Manually check whether the file exists.  This is prone to race
     conditions, but systems without O_EXCL haven't deserved
     better.  */
  if (file_exists_p (fname))
    {
      errno = EEXIST;
      return NULL;
    }
  return fopen (fname, binary ? "wb" : "w");
#endif /* not O_EXCL */
}

/* Create DIRECTORY.  If some of the pathname components of DIRECTORY
   are missing, create them first.  In case any mkdir() call fails,
   return its error status.  Returns 0 on successful completion.

   The behaviour of this function should be identical to the behaviour
   of `mkdir -p' on systems where mkdir supports the `-p' option.  */
int
make_directory (const char *directory)
{
  int i, ret, quit = 0;
  char *dir;

  /* Make a copy of dir, to be able to write to it.  Otherwise, the
     function is unsafe if called with a read-only char *argument.  */
  STRDUP_ALLOCA (dir, directory);

  /* If the first character of dir is '/', skip it (and thus enable
     creation of absolute-pathname directories.  */
  for (i = (*dir == '/'); 1; ++i)
    {
      for (; dir[i] && dir[i] != '/'; i++)
	;
      if (!dir[i])
	quit = 1;
      dir[i] = '\0';
      /* Check whether the directory already exists.  Allow creation of
	 of intermediate directories to fail, as the initial path components
	 are not necessarily directories!  */
      if (!file_exists_p (dir))
	ret = mkdir (dir, 0777);
      else
	ret = 0;
      if (quit)
	break;
      else
	dir[i] = '/';
    }
  return ret;
}

/* Merge BASE with FILE.  BASE can be a directory or a file name, FILE
   should be a file name.

   file_merge("/foo/bar", "baz")  => "/foo/baz"
   file_merge("/foo/bar/", "baz") => "/foo/bar/baz"
   file_merge("foo", "bar")       => "bar"

   In other words, it's a simpler and gentler version of uri_merge_1.  */

char *
file_merge (const char *base, const char *file)
{
  char *result;
  const char *cut = (const char *)strrchr (base, '/');

  if (!cut)
    return xstrdup (file);

  result = (char *)xmalloc (cut - base + 1 + strlen (file) + 1);
  memcpy (result, base, cut - base);
  result[cut - base] = '/';
  strcpy (result + (cut - base) + 1, file);

  return result;
}

static int in_acclist PARAMS ((const char *const *, const char *, int));

/* Determine whether a file is acceptable to be followed, according to
   lists of patterns to accept/reject.  */
int
acceptable (const char *s)
{
  int l = strlen (s);

  while (l && s[l] != '/')
    --l;
  if (s[l] == '/')
    s += (l + 1);
  if (opt.accepts)
    {
      if (opt.rejects)
	return (in_acclist ((const char *const *)opt.accepts, s, 1)
		&& !in_acclist ((const char *const *)opt.rejects, s, 1));
      else
	return in_acclist ((const char *const *)opt.accepts, s, 1);
    }
  else if (opt.rejects)
    return !in_acclist ((const char *const *)opt.rejects, s, 1);
  return 1;
}

/* Compare S1 and S2 frontally; S2 must begin with S1.  E.g. if S1 is
   `/something', frontcmp() will return 1 only if S2 begins with
   `/something'.  Otherwise, 0 is returned.  */
int
frontcmp (const char *s1, const char *s2)
{
  for (; *s1 && *s2 && (*s1 == *s2); ++s1, ++s2);
  return !*s1;
}

/* Iterate through STRLIST, and return the first element that matches
   S, through wildcards or front comparison (as appropriate).  */
static char *
proclist (char **strlist, const char *s, enum accd flags)
{
  char **x;
  for (x = strlist; *x; x++)
    {
      /* Remove leading '/' if ALLABS */
      char *p = *x + ((flags & ALLABS) && (**x == '/'));
      if (has_wildcards_p (p))
	{
	  if (fnmatch (p, s, FNM_PATHNAME) == 0)
	    break;
	}
      else
	{
	  if (frontcmp (p, s))
	    break;
	}
    }
  return *x;
}

/* Returns whether DIRECTORY is acceptable for download, wrt the
   include/exclude lists.

   If FLAGS is ALLABS, the leading `/' is ignored in paths; relative
   and absolute paths may be freely intermixed.  */
int
accdir (const char *directory, enum accd flags)
{
  /* Remove starting '/'.  */
  if (flags & ALLABS && *directory == '/')
    ++directory;
  if (opt.includes)
    {
      if (!proclist (opt.includes, directory, flags))
	return 0;
    }
  if (opt.excludes)
    {
      if (proclist (opt.excludes, directory, flags))
	return 0;
    }
  return 1;
}

/* Return non-zero if STRING ends with TAIL.  For instance:

   match_tail ("abc", "bc", 0)  -> 1
   match_tail ("abc", "ab", 0)  -> 0
   match_tail ("abc", "abc", 0) -> 1

   If FOLD_CASE_P is non-zero, the comparison will be
   case-insensitive.  */

int
match_tail (const char *string, const char *tail, int fold_case_p)
{
  int i, j;

  /* We want this to be fast, so we code two loops, one with
     case-folding, one without. */

  if (!fold_case_p)
    {
      for (i = strlen (string), j = strlen (tail); i >= 0 && j >= 0; i--, j--)
	if (string[i] != tail[j])
	  break;
    }
  else
    {
      for (i = strlen (string), j = strlen (tail); i >= 0 && j >= 0; i--, j--)
	if (TOLOWER (string[i]) != TOLOWER (tail[j]))
	  break;
    }

  /* If the tail was exhausted, the match was succesful.  */
  if (j == -1)
    return 1;
  else
    return 0;
}

/* Checks whether string S matches each element of ACCEPTS.  A list
   element are matched either with fnmatch() or match_tail(),
   according to whether the element contains wildcards or not.

   If the BACKWARD is 0, don't do backward comparison -- just compare
   them normally.  */
static int
in_acclist (const char *const *accepts, const char *s, int backward)
{
  for (; *accepts; accepts++)
    {
      if (has_wildcards_p (*accepts))
	{
	  /* fnmatch returns 0 if the pattern *does* match the
	     string.  */
	  if (fnmatch (*accepts, s, 0) == 0)
	    return 1;
	}
      else
	{
	  if (backward)
	    {
	      if (match_tail (s, *accepts, 0))
		return 1;
	    }
	  else
	    {
	      if (!strcmp (s, *accepts))
		return 1;
	    }
	}
    }
  return 0;
}

/* Return the location of STR's suffix (file extension).  Examples:
   suffix ("foo.bar")       -> "bar"
   suffix ("foo.bar.baz")   -> "baz"
   suffix ("/foo/bar")      -> NULL
   suffix ("/foo.bar/baz")  -> NULL  */
char *
suffix (const char *str)
{
  int i;

  for (i = strlen (str); i && str[i] != '/' && str[i] != '.'; i--)
    ;

  if (str[i++] == '.')
    return (char *)str + i;
  else
    return NULL;
}

/* Return non-zero if S contains globbing wildcards (`*', `?', `[' or
   `]').  */

int
has_wildcards_p (const char *s)
{
  for (; *s; s++)
    if (*s == '*' || *s == '?' || *s == '[' || *s == ']')
      return 1;
  return 0;
}

/* Return non-zero if FNAME ends with a typical HTML suffix.  The
   following (case-insensitive) suffixes are presumed to be HTML files:
   
     html
     htm
     ?html (`?' matches one character)

   #### CAVEAT.  This is not necessarily a good indication that FNAME
   refers to a file that contains HTML!  */
int
has_html_suffix_p (const char *fname)
{
  char *suf;

  if ((suf = suffix (fname)) == NULL)
    return 0;
  if (!strcasecmp (suf, "html"))
    return 1;
  if (!strcasecmp (suf, "htm"))
    return 1;
  if (suf[0] && !strcasecmp (suf + 1, "html"))
    return 1;
  return 0;
}

/* Read a line from FP and return the pointer to freshly allocated
   storage.  The storage space is obtained through malloc() and should
   be freed with free() when it is no longer needed.

   The length of the line is not limited, except by available memory.
   The newline character at the end of line is retained.  The line is
   terminated with a zero character.

   After end-of-file is encountered without anything being read, NULL
   is returned.  NULL is also returned on error.  To distinguish
   between these two cases, use the stdio function ferror().  */

char *
read_whole_line (FILE *fp)
{
  int length = 0;
  int bufsize = 82;
  char *line = (char *)xmalloc (bufsize);

  while (fgets (line + length, bufsize - length, fp))
    {
      length += strlen (line + length);
      if (length == 0)
	/* Possible for example when reading from a binary file where
	   a line begins with \0.  */
	continue;

      if (line[length - 1] == '\n')
	break;

      /* fgets() guarantees to read the whole line, or to use up the
         space we've given it.  We can double the buffer
         unconditionally.  */
      bufsize <<= 1;
      line = xrealloc (line, bufsize);
    }
  if (length == 0 || ferror (fp))
    {
      xfree (line);
      return NULL;
    }
  if (length + 1 < bufsize)
    /* Relieve the memory from our exponential greediness.  We say
       `length + 1' because the terminating \0 is not included in
       LENGTH.  We don't need to zero-terminate the string ourselves,
       though, because fgets() does that.  */
    line = xrealloc (line, length + 1);
  return line;
}

/* Read FILE into memory.  A pointer to `struct file_memory' are
   returned; use struct element `content' to access file contents, and
   the element `length' to know the file length.  `content' is *not*
   zero-terminated, and you should *not* read or write beyond the [0,
   length) range of characters.

   After you are done with the file contents, call read_file_free to
   release the memory.

   Depending on the operating system and the type of file that is
   being read, read_file() either mmap's the file into memory, or
   reads the file into the core using read().

   If file is named "-", fileno(stdin) is used for reading instead.
   If you want to read from a real file named "-", use "./-" instead.  */

struct file_memory *
read_file (const char *file)
{
  int fd;
  struct file_memory *fm;
  long size;
  int inhibit_close = 0;

  /* Some magic in the finest tradition of Perl and its kin: if FILE
     is "-", just use stdin.  */
  if (HYPHENP (file))
    {
      fd = fileno (stdin);
      inhibit_close = 1;
      /* Note that we don't inhibit mmap() in this case.  If stdin is
         redirected from a regular file, mmap() will still work.  */
    }
  else
    fd = open (file, O_RDONLY);
  if (fd < 0)
    return NULL;
  fm = xnew (struct file_memory);

#ifdef HAVE_MMAP
  {
    struct_fstat buf;
    if (fstat (fd, &buf) < 0)
      goto mmap_lose;
    fm->length = buf.st_size;
    /* NOTE: As far as I know, the callers of this function never
       modify the file text.  Relying on this would enable us to
       specify PROT_READ and MAP_SHARED for a marginal gain in
       efficiency, but at some cost to generality.  */
    fm->content = mmap (NULL, fm->length, PROT_READ | PROT_WRITE,
			MAP_PRIVATE, fd, 0);
    if (fm->content == (char *)MAP_FAILED)
      goto mmap_lose;
    if (!inhibit_close)
      close (fd);

    fm->mmap_p = 1;
    return fm;
  }

 mmap_lose:
  /* The most common reason why mmap() fails is that FD does not point
     to a plain file.  However, it's also possible that mmap() doesn't
     work for a particular type of file.  Therefore, whenever mmap()
     fails, we just fall back to the regular method.  */
#endif /* HAVE_MMAP */

  fm->length = 0;
  size = 512;			/* number of bytes fm->contents can
                                   hold at any given time. */
  fm->content = xmalloc (size);
  while (1)
    {
      wgint nread;
      if (fm->length > size / 2)
	{
	  /* #### I'm not sure whether the whole exponential-growth
             thing makes sense with kernel read.  On Linux at least,
             read() refuses to read more than 4K from a file at a
             single chunk anyway.  But other Unixes might optimize it
             better, and it doesn't *hurt* anything, so I'm leaving
             it.  */

	  /* Normally, we grow SIZE exponentially to make the number
             of calls to read() and realloc() logarithmic in relation
             to file size.  However, read() can read an amount of data
             smaller than requested, and it would be unreasonable to
             double SIZE every time *something* was read.  Therefore,
             we double SIZE only when the length exceeds half of the
             entire allocated size.  */
	  size <<= 1;
	  fm->content = xrealloc (fm->content, size);
	}
      nread = read (fd, fm->content + fm->length, size - fm->length);
      if (nread > 0)
	/* Successful read. */
	fm->length += nread;
      else if (nread < 0)
	/* Error. */
	goto lose;
      else
	/* EOF */
	break;
    }
  if (!inhibit_close)
    close (fd);
  if (size > fm->length && fm->length != 0)
    /* Due to exponential growth of fm->content, the allocated region
       might be much larger than what is actually needed.  */
    fm->content = xrealloc (fm->content, fm->length);
  fm->mmap_p = 0;
  return fm;

 lose:
  if (!inhibit_close)
    close (fd);
  xfree (fm->content);
  xfree (fm);
  return NULL;
}

/* Release the resources held by FM.  Specifically, this calls
   munmap() or xfree() on fm->content, depending whether mmap or
   malloc/read were used to read in the file.  It also frees the
   memory needed to hold the FM structure itself.  */

void
read_file_free (struct file_memory *fm)
{
#ifdef HAVE_MMAP
  if (fm->mmap_p)
    {
      munmap (fm->content, fm->length);
    }
  else
#endif
    {
      xfree (fm->content);
    }
  xfree (fm);
}

/* Free the pointers in a NULL-terminated vector of pointers, then
   free the pointer itself.  */
void
free_vec (char **vec)
{
  if (vec)
    {
      char **p = vec;
      while (*p)
	xfree (*p++);
      xfree (vec);
    }
}

/* Append vector V2 to vector V1.  The function frees V2 and
   reallocates V1 (thus you may not use the contents of neither
   pointer after the call).  If V1 is NULL, V2 is returned.  */
char **
merge_vecs (char **v1, char **v2)
{
  int i, j;

  if (!v1)
    return v2;
  if (!v2)
    return v1;
  if (!*v2)
    {
      /* To avoid j == 0 */
      xfree (v2);
      return v1;
    }
  /* Count v1.  */
  for (i = 0; v1[i]; i++);
  /* Count v2.  */
  for (j = 0; v2[j]; j++);
  /* Reallocate v1.  */
  v1 = (char **)xrealloc (v1, (i + j + 1) * sizeof (char **));
  memcpy (v1 + i, v2, (j + 1) * sizeof (char *));
  xfree (v2);
  return v1;
}

/* Append a freshly allocated copy of STR to VEC.  If VEC is NULL, it
   is allocated as needed.  Return the new value of the vector. */

char **
vec_append (char **vec, const char *str)
{
  int cnt;			/* count of vector elements, including
				   the one we're about to append */
  if (vec != NULL)
    {
      for (cnt = 0; vec[cnt]; cnt++)
	;
      ++cnt;
    }
  else
    cnt = 1;
  /* Reallocate the array to fit the new element and the NULL. */
  vec = xrealloc (vec, (cnt + 1) * sizeof (char *));
  /* Append a copy of STR to the vector. */
  vec[cnt - 1] = xstrdup (str);
  vec[cnt] = NULL;
  return vec;
}

/* Sometimes it's useful to create "sets" of strings, i.e. special
   hash tables where you want to store strings as keys and merely
   query for their existence.  Here is a set of utility routines that
   makes that transparent.  */

void
string_set_add (struct hash_table *ht, const char *s)
{
  /* First check whether the set element already exists.  If it does,
     do nothing so that we don't have to free() the old element and
     then strdup() a new one.  */
  if (hash_table_contains (ht, s))
    return;

  /* We use "1" as value.  It provides us a useful and clear arbitrary
     value, and it consumes no memory -- the pointers to the same
     string "1" will be shared by all the key-value pairs in all `set'
     hash tables.  */
  hash_table_put (ht, xstrdup (s), "1");
}

/* Synonym for hash_table_contains... */

int
string_set_contains (struct hash_table *ht, const char *s)
{
  return hash_table_contains (ht, s);
}

static int
string_set_to_array_mapper (void *key, void *value_ignored, void *arg)
{
  char ***arrayptr = (char ***) arg;
  *(*arrayptr)++ = (char *) key;
  return 0;
}

/* Convert the specified string set to array.  ARRAY should be large
   enough to hold hash_table_count(ht) char pointers.  */

void string_set_to_array (struct hash_table *ht, char **array)
{
  hash_table_map (ht, string_set_to_array_mapper, &array);
}

static int
string_set_free_mapper (void *key, void *value_ignored, void *arg_ignored)
{
  xfree (key);
  return 0;
}

void
string_set_free (struct hash_table *ht)
{
  hash_table_map (ht, string_set_free_mapper, NULL);
  hash_table_destroy (ht);
}

static int
free_keys_and_values_mapper (void *key, void *value, void *arg_ignored)
{
  xfree (key);
  xfree (value);
  return 0;
}

/* Another utility function: call free() on all keys and values of HT.  */

void
free_keys_and_values (struct hash_table *ht)
{
  hash_table_map (ht, free_keys_and_values_mapper, NULL);
}

static void
get_grouping_data (const char **sep, const char **grouping)
{
  static const char *cached_sep;
  static const char *cached_grouping;
  static int initialized;
  if (!initialized)
    {
      /* If locale.h is present and defines LC_NUMERIC, assume C89
	 struct lconv with "thousand_sep" and "grouping" members.  */
#ifdef LC_NUMERIC
      /* Get the grouping info from the locale. */
      struct lconv *lconv;
      const char *oldlocale = setlocale (LC_NUMERIC, NULL);
      /* Temporarily switch to the current locale */
      setlocale (LC_NUMERIC, "");
      lconv = localeconv ();
      cached_sep = xstrdup (lconv->thousands_sep);
      cached_grouping = xstrdup (lconv->grouping);
      /* Restore the locale to previous setting. */
      setlocale (LC_NUMERIC, oldlocale);
      if (!*cached_sep)
#endif
	/* Force separator for locales that specify no separators
	   ("C", "hr", and probably many more.) */
	cached_sep = ",", cached_grouping = "\x03";
      initialized = 1;
    }
  *sep = cached_sep;
  *grouping = cached_grouping;
}

/* Add thousand separators to a number already in string form.  Used
   by with_thousand_seps and with_thousand_seps_sum.  */

char *
add_thousand_seps (const char *repr)
{
  static char outbuf[48];
  char *p = outbuf + sizeof outbuf;

  const char *in = strchr (repr, '\0');
  const char *instart = repr + (*repr == '-'); /* don't group sign */

  /* Info received from locale */
  const char *grouping, *sep;
  int seplen;

  /* State information */
  int i = 0, groupsize;
  const char *atgroup;

  /* Initialize grouping data. */
  get_grouping_data (&sep, &grouping);
  seplen = strlen (sep);
  atgroup = grouping;
  groupsize = *atgroup++;

  /* Write the number into the buffer, backwards, inserting the
     separators as necessary.  */
  *--p = '\0';
  while (1)
    {
      *--p = *--in;
      if (in == instart)
	break;
      /* Prepend SEP to every groupsize'd digit and get new groupsize.  */
      if (++i == groupsize)
	{
	  if (seplen == 1)
	    *--p = *sep;
	  else
	    memcpy (p -= seplen, sep, seplen);
	  i = 0;
	  if (*atgroup)
	    groupsize = *atgroup++;
	}
    }
  if (*repr == '-')
    *--p = '-';

  return p;
}

/* Return a printed representation of N with thousand separators.
   This should respect locale settings, with the exception of the "C"
   locale which mandates no separator, but we use one anyway.

   Unfortunately, we cannot use %'d (in fact it would be %'j) to get
   the separators because it's too non-portable, and it's hard to test
   for this feature at configure time.  Besides, it wouldn't work in
   the "C" locale, which many Unix users still work in.  */

char *
with_thousand_seps (wgint l)
{
  char inbuf[24];
  /* Print the number into the buffer.  */
  number_to_string (inbuf, l);
  return add_thousand_seps (inbuf);
}

/* When SUM_SIZE_INT is wgint, with_thousand_seps_large is #defined to
   with_thousand_seps.  The function below is used on non-LFS systems
   where SUM_SIZE_INT typedeffed to double.  */

#ifndef with_thousand_seps_sum
char *
with_thousand_seps_sum (SUM_SIZE_INT l)
{
  char inbuf[32];
  snprintf (inbuf, sizeof (inbuf), "%.0f", l);
  return add_thousand_seps (inbuf);
}
#endif /* not with_thousand_seps_sum */

/* N, a byte quantity, is converted to a human-readable abberviated
   form a la sizes printed by `ls -lh'.  The result is written to a
   static buffer, a pointer to which is returned.

   Unlike `with_thousand_seps', this approximates to the nearest unit.
   Quoting GNU libit: "Most people visually process strings of 3-4
   digits effectively, but longer strings of digits are more prone to
   misinterpretation.  Hence, converting to an abbreviated form
   usually improves readability."

   This intentionally uses kilobyte (KB), megabyte (MB), etc. in their
   original computer science meaning of "powers of 1024".  Powers of
   1000 would be useless since Wget already displays sizes with
   thousand separators.  We don't use the "*bibyte" names invented in
   1998, and seldom used in practice.  Wikipedia's entry on kilobyte
   discusses this in some detail.  */

char *
human_readable (wgint n)
{
  /* These suffixes are compatible with those of GNU `ls -lh'. */
  static char powers[] =
    {
      'K',			/* kilobyte, 2^10 bytes */
      'M',			/* megabyte, 2^20 bytes */
      'G',			/* gigabyte, 2^30 bytes */
      'T',			/* terabyte, 2^40 bytes */
      'P',			/* petabyte, 2^50 bytes */
      'E',			/* exabyte,  2^60 bytes */
    };
  static char buf[8];
  int i;

  /* If the quantity is smaller than 1K, just print it. */
  if (n < 1024)
    {
      snprintf (buf, sizeof (buf), "%d", (int) n);
      return buf;
    }

  /* Loop over powers, dividing N with 1024 in each iteration.  This
     works unchanged for all sizes of wgint, while still avoiding
     non-portable `long double' arithmetic.  */
  for (i = 0; i < countof (powers); i++)
    {
      /* At each iteration N is greater than the *subsequent* power.
	 That way N/1024.0 produces a decimal number in the units of
	 *this* power.  */
      if ((n >> 10) < 1024 || i == countof (powers) - 1)
	{
	  double val = n / 1024.0;
	  /* Print values smaller than 10 with one decimal digits, and
	     others without any decimals.  */
	  snprintf (buf, sizeof (buf), "%.*f%c",
		    val < 10 ? 1 : 0, val, powers[i]);
	  return buf;
	}
      n >>= 10;
    }
  return NULL;			/* unreached */
}

/* Count the digits in the provided number.  Used to allocate space
   when printing numbers.  */

int
numdigit (wgint number)
{
  int cnt = 1;
  if (number < 0)
    ++cnt;			/* accomodate '-' */
  while ((number /= 10) != 0)
    ++cnt;
  return cnt;
}

#define PR(mask) *p++ = n / (mask) + '0'

/* DIGITS_<D> is used to print a D-digit number and should be called
   with mask==10^(D-1).  It prints n/mask (the first digit), reducing
   n to n%mask (the remaining digits), and calling DIGITS_<D-1>.
   Recursively this continues until DIGITS_1 is invoked.  */

#define DIGITS_1(mask) PR (mask)
#define DIGITS_2(mask) PR (mask), n %= (mask), DIGITS_1 ((mask) / 10)
#define DIGITS_3(mask) PR (mask), n %= (mask), DIGITS_2 ((mask) / 10)
#define DIGITS_4(mask) PR (mask), n %= (mask), DIGITS_3 ((mask) / 10)
#define DIGITS_5(mask) PR (mask), n %= (mask), DIGITS_4 ((mask) / 10)
#define DIGITS_6(mask) PR (mask), n %= (mask), DIGITS_5 ((mask) / 10)
#define DIGITS_7(mask) PR (mask), n %= (mask), DIGITS_6 ((mask) / 10)
#define DIGITS_8(mask) PR (mask), n %= (mask), DIGITS_7 ((mask) / 10)
#define DIGITS_9(mask) PR (mask), n %= (mask), DIGITS_8 ((mask) / 10)
#define DIGITS_10(mask) PR (mask), n %= (mask), DIGITS_9 ((mask) / 10)

/* DIGITS_<11-20> are only used on machines with 64-bit wgints. */

#define DIGITS_11(mask) PR (mask), n %= (mask), DIGITS_10 ((mask) / 10)
#define DIGITS_12(mask) PR (mask), n %= (mask), DIGITS_11 ((mask) / 10)
#define DIGITS_13(mask) PR (mask), n %= (mask), DIGITS_12 ((mask) / 10)
#define DIGITS_14(mask) PR (mask), n %= (mask), DIGITS_13 ((mask) / 10)
#define DIGITS_15(mask) PR (mask), n %= (mask), DIGITS_14 ((mask) / 10)
#define DIGITS_16(mask) PR (mask), n %= (mask), DIGITS_15 ((mask) / 10)
#define DIGITS_17(mask) PR (mask), n %= (mask), DIGITS_16 ((mask) / 10)
#define DIGITS_18(mask) PR (mask), n %= (mask), DIGITS_17 ((mask) / 10)
#define DIGITS_19(mask) PR (mask), n %= (mask), DIGITS_18 ((mask) / 10)

/* SPRINTF_WGINT is used by number_to_string to handle pathological
   cases and to portably support strange sizes of wgint.  Ideally this
   would just use "%j" and intmax_t, but many systems don't support
   it, so it's used only if nothing else works.  */
#if SIZEOF_LONG >= SIZEOF_WGINT
#  define SPRINTF_WGINT(buf, n) sprintf (buf, "%ld", (long) (n))
#else
# if SIZEOF_LONG_LONG >= SIZEOF_WGINT
#   define SPRINTF_WGINT(buf, n) sprintf (buf, "%lld", (long long) (n))
# else
#  ifdef WINDOWS
#   define SPRINTF_WGINT(buf, n) sprintf (buf, "%I64d", (__int64) (n))
#  else
#   define SPRINTF_WGINT(buf, n) sprintf (buf, "%j", (intmax_t) (n))
#  endif
# endif
#endif

/* Shorthand for casting to wgint. */
#define W wgint

/* Print NUMBER to BUFFER in base 10.  This is equivalent to
   `sprintf(buffer, "%lld", (long long) number)', only typically much
   faster and portable to machines without long long.

   The speedup may make a difference in programs that frequently
   convert numbers to strings.  Some implementations of sprintf,
   particularly the one in GNU libc, have been known to be extremely
   slow when converting integers to strings.

   Return the pointer to the location where the terminating zero was
   printed.  (Equivalent to calling buffer+strlen(buffer) after the
   function is done.)

   BUFFER should be big enough to accept as many bytes as you expect
   the number to take up.  On machines with 64-bit longs the maximum
   needed size is 24 bytes.  That includes the digits needed for the
   largest 64-bit number, the `-' sign in case it's negative, and the
   terminating '\0'.  */

char *
number_to_string (char *buffer, wgint number)
{
  char *p = buffer;
  wgint n = number;

#if (SIZEOF_WGINT != 4) && (SIZEOF_WGINT != 8)
  /* We are running in a strange or misconfigured environment.  Let
     sprintf cope with it.  */
  SPRINTF_WGINT (buffer, n);
  p += strlen (buffer);
#else  /* (SIZEOF_WGINT == 4) || (SIZEOF_WGINT == 8) */

  if (n < 0)
    {
      if (n < -WGINT_MAX)
	{
	  /* -n would overflow.  Have sprintf deal with this.  */
	  SPRINTF_WGINT (buffer, n);
	  p += strlen (buffer);
	  return p;
	}

      *p++ = '-';
      n = -n;
    }

  /* Use the DIGITS_ macro appropriate for N's number of digits.  That
     way printing any N is fully open-coded without a loop or jump.
     (Also see description of DIGITS_*.)  */

  if      (n < 10)                       DIGITS_1 (1);
  else if (n < 100)                      DIGITS_2 (10);
  else if (n < 1000)                     DIGITS_3 (100);
  else if (n < 10000)                    DIGITS_4 (1000);
  else if (n < 100000)                   DIGITS_5 (10000);
  else if (n < 1000000)                  DIGITS_6 (100000);
  else if (n < 10000000)                 DIGITS_7 (1000000);
  else if (n < 100000000)                DIGITS_8 (10000000);
  else if (n < 1000000000)               DIGITS_9 (100000000);
#if SIZEOF_WGINT == 4
  /* wgint is 32 bits wide: no number has more than 10 digits. */
  else                                   DIGITS_10 (1000000000);
#else
  /* wgint is 64 bits wide: handle numbers with more than 9 decimal
     digits.  Constants are constructed by compile-time multiplication
     to avoid dealing with different notations for 64-bit constants
     (nnnL, nnnLL, and nnnI64, depending on the compiler).  */
  else if (n < 10*(W)1000000000)         DIGITS_10 (1000000000);
  else if (n < 100*(W)1000000000)        DIGITS_11 (10*(W)1000000000);
  else if (n < 1000*(W)1000000000)       DIGITS_12 (100*(W)1000000000);
  else if (n < 10000*(W)1000000000)      DIGITS_13 (1000*(W)1000000000);
  else if (n < 100000*(W)1000000000)     DIGITS_14 (10000*(W)1000000000);
  else if (n < 1000000*(W)1000000000)    DIGITS_15 (100000*(W)1000000000);
  else if (n < 10000000*(W)1000000000)   DIGITS_16 (1000000*(W)1000000000);
  else if (n < 100000000*(W)1000000000)  DIGITS_17 (10000000*(W)1000000000);
  else if (n < 1000000000*(W)1000000000) DIGITS_18 (100000000*(W)1000000000);
  else                                   DIGITS_19 (1000000000*(W)1000000000);
#endif

  *p = '\0';
#endif /* (SIZEOF_WGINT == 4) || (SIZEOF_WGINT == 8) */

  return p;
}

#undef PR
#undef W
#undef DIGITS_1
#undef DIGITS_2
#undef DIGITS_3
#undef DIGITS_4
#undef DIGITS_5
#undef DIGITS_6
#undef DIGITS_7
#undef DIGITS_8
#undef DIGITS_9
#undef DIGITS_10
#undef DIGITS_11
#undef DIGITS_12
#undef DIGITS_13
#undef DIGITS_14
#undef DIGITS_15
#undef DIGITS_16
#undef DIGITS_17
#undef DIGITS_18
#undef DIGITS_19

#define RING_SIZE 3

/* Print NUMBER to a statically allocated string and return a pointer
   to the printed representation.

   This function is intended to be used in conjunction with printf.
   It is hard to portably print wgint values:
    a) you cannot use printf("%ld", number) because wgint can be long
       long on 32-bit machines with LFS.
    b) you cannot use printf("%lld", number) because NUMBER could be
       long on 32-bit machines without LFS, or on 64-bit machines,
       which do not require LFS.  Also, Windows doesn't support %lld.
    c) you cannot use printf("%j", (int_max_t) number) because not all
       versions of printf support "%j", the most notable being the one
       on Windows.
    d) you cannot #define WGINT_FMT to the appropriate format and use
       printf(WGINT_FMT, number) because that would break translations
       for user-visible messages, such as printf("Downloaded: %d
       bytes\n", number).

   What you should use instead is printf("%s", number_to_static_string
   (number)).

   CAVEAT: since the function returns pointers to static data, you
   must be careful to copy its result before calling it again.
   However, to make it more useful with printf, the function maintains
   an internal ring of static buffers to return.  That way things like
   printf("%s %s", number_to_static_string (num1),
   number_to_static_string (num2)) work as expected.  Three buffers
   are currently used, which means that "%s %s %s" will work, but "%s
   %s %s %s" won't.  If you need to print more than three wgints,
   bump the RING_SIZE (or rethink your message.)  */

char *
number_to_static_string (wgint number)
{
  static char ring[RING_SIZE][24];
  static int ringpos;
  char *buf = ring[ringpos];
  number_to_string (buf, number);
  ringpos = (ringpos + 1) % RING_SIZE;
  return buf;
}

/* Determine the width of the terminal we're running on.  If that's
   not possible, return 0.  */

int
determine_screen_width (void)
{
  /* If there's a way to get the terminal size using POSIX
     tcgetattr(), somebody please tell me.  */
#ifdef TIOCGWINSZ
  int fd;
  struct winsize wsz;

  if (opt.lfilename != NULL)
    return 0;

  fd = fileno (stderr);
  if (ioctl (fd, TIOCGWINSZ, &wsz) < 0)
    return 0;			/* most likely ENOTTY */

  return wsz.ws_col;
#else  /* not TIOCGWINSZ */
# ifdef WINDOWS
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  if (!GetConsoleScreenBufferInfo (GetStdHandle (STD_ERROR_HANDLE), &csbi))
    return 0;
  return csbi.dwSize.X;
# else /* neither WINDOWS nor TIOCGWINSZ */
  return 0;
#endif /* neither WINDOWS nor TIOCGWINSZ */
#endif /* not TIOCGWINSZ */
}

/* Return a random number between 0 and MAX-1, inclusive.

   If MAX is greater than the value of RAND_MAX+1 on the system, the
   returned value will be in the range [0, RAND_MAX].  This may be
   fixed in a future release.

   The random number generator is seeded automatically the first time
   it is called.

   This uses rand() for portability.  It has been suggested that
   random() offers better randomness, but this is not required for
   Wget, so I chose to go for simplicity and use rand
   unconditionally.

   DO NOT use this for cryptographic purposes.  It is only meant to be
   used in situations where quality of the random numbers returned
   doesn't really matter.  */

int
random_number (int max)
{
  static int seeded;
  double bounded;
  int rnd;

  if (!seeded)
    {
      srand (time (NULL));
      seeded = 1;
    }
  rnd = rand ();

  /* On systems that don't define RAND_MAX, assume it to be 2**15 - 1,
     and enforce that assumption by masking other bits.  */
#ifndef RAND_MAX
# define RAND_MAX 32767
  rnd &= RAND_MAX;
#endif

  /* This is equivalent to rand() % max, but uses the high-order bits
     for better randomness on architecture where rand() is implemented
     using a simple congruential generator.  */

  bounded = (double)max * rnd / (RAND_MAX + 1.0);
  return (int)bounded;
}

/* Return a random uniformly distributed floating point number in the
   [0, 1) range.  The precision of returned numbers is 9 digits.

   Modify this to use erand48() where available!  */

double
random_float (void)
{
  /* We can't rely on any specific value of RAND_MAX, but I'm pretty
     sure it's greater than 1000.  */
  int rnd1 = random_number (1000);
  int rnd2 = random_number (1000);
  int rnd3 = random_number (1000);
  return rnd1 / 1000.0 + rnd2 / 1000000.0 + rnd3 / 1000000000.0;
}

/* Implementation of run_with_timeout, a generic timeout-forcing
   routine for systems with Unix-like signal handling.  */

#ifdef USE_SIGNAL_TIMEOUT
# ifdef HAVE_SIGSETJMP
#  define SETJMP(env) sigsetjmp (env, 1)

static sigjmp_buf run_with_timeout_env;

static RETSIGTYPE
abort_run_with_timeout (int sig)
{
  assert (sig == SIGALRM);
  siglongjmp (run_with_timeout_env, -1);
}
# else /* not HAVE_SIGSETJMP */
#  define SETJMP(env) setjmp (env)

static jmp_buf run_with_timeout_env;

static RETSIGTYPE
abort_run_with_timeout (int sig)
{
  assert (sig == SIGALRM);
  /* We don't have siglongjmp to preserve the set of blocked signals;
     if we longjumped out of the handler at this point, SIGALRM would
     remain blocked.  We must unblock it manually. */
  int mask = siggetmask ();
  mask &= ~sigmask (SIGALRM);
  sigsetmask (mask);

  /* Now it's safe to longjump. */
  longjmp (run_with_timeout_env, -1);
}
# endif /* not HAVE_SIGSETJMP */

/* Arrange for SIGALRM to be delivered in TIMEOUT seconds.  This uses
   setitimer where available, alarm otherwise.

   TIMEOUT should be non-zero.  If the timeout value is so small that
   it would be rounded to zero, it is rounded to the least legal value
   instead (1us for setitimer, 1s for alarm).  That ensures that
   SIGALRM will be delivered in all cases.  */

static void
alarm_set (double timeout)
{
#ifdef ITIMER_REAL
  /* Use the modern itimer interface. */
  struct itimerval itv;
  xzero (itv);
  itv.it_value.tv_sec = (long) timeout;
  itv.it_value.tv_usec = 1000000 * (timeout - (long)timeout);
  if (itv.it_value.tv_sec == 0 && itv.it_value.tv_usec == 0)
    /* Ensure that we wait for at least the minimum interval.
       Specifying zero would mean "wait forever".  */
    itv.it_value.tv_usec = 1;
  setitimer (ITIMER_REAL, &itv, NULL);
#else  /* not ITIMER_REAL */
  /* Use the old alarm() interface. */
  int secs = (int) timeout;
  if (secs == 0)
    /* Round TIMEOUTs smaller than 1 to 1, not to zero.  This is
       because alarm(0) means "never deliver the alarm", i.e. "wait
       forever", which is not what someone who specifies a 0.5s
       timeout would expect.  */
    secs = 1;
  alarm (secs);
#endif /* not ITIMER_REAL */
}

/* Cancel the alarm set with alarm_set. */

static void
alarm_cancel (void)
{
#ifdef ITIMER_REAL
  struct itimerval disable;
  xzero (disable);
  setitimer (ITIMER_REAL, &disable, NULL);
#else  /* not ITIMER_REAL */
  alarm (0);
#endif /* not ITIMER_REAL */
}

/* Call FUN(ARG), but don't allow it to run for more than TIMEOUT
   seconds.  Returns non-zero if the function was interrupted with a
   timeout, zero otherwise.

   This works by setting up SIGALRM to be delivered in TIMEOUT seconds
   using setitimer() or alarm().  The timeout is enforced by
   longjumping out of the SIGALRM handler.  This has several
   advantages compared to the traditional approach of relying on
   signals causing system calls to exit with EINTR:

     * The callback function is *forcibly* interrupted after the
       timeout expires, (almost) regardless of what it was doing and
       whether it was in a syscall.  For example, a calculation that
       takes a long time is interrupted as reliably as an IO
       operation.

     * It works with both SYSV and BSD signals because it doesn't
       depend on the default setting of SA_RESTART.

     * It doesn't require special handler setup beyond a simple call
       to signal().  (It does use sigsetjmp/siglongjmp, but they're
       optional.)

   The only downside is that, if FUN allocates internal resources that
   are normally freed prior to exit from the functions, they will be
   lost in case of timeout.  */

int
run_with_timeout (double timeout, void (*fun) (void *), void *arg)
{
  int saved_errno;

  if (timeout == 0)
    {
      fun (arg);
      return 0;
    }

  signal (SIGALRM, abort_run_with_timeout);
  if (SETJMP (run_with_timeout_env) != 0)
    {
      /* Longjumped out of FUN with a timeout. */
      signal (SIGALRM, SIG_DFL);
      return 1;
    }
  alarm_set (timeout);
  fun (arg);

  /* Preserve errno in case alarm() or signal() modifies it. */
  saved_errno = errno;
  alarm_cancel ();
  signal (SIGALRM, SIG_DFL);
  errno = saved_errno;

  return 0;
}

#else  /* not USE_SIGNAL_TIMEOUT */

#ifndef WINDOWS
/* A stub version of run_with_timeout that just calls FUN(ARG).  Don't
   define it under Windows, because Windows has its own version of
   run_with_timeout that uses threads.  */

int
run_with_timeout (double timeout, void (*fun) (void *), void *arg)
{
  fun (arg);
  return 0;
}
#endif /* not WINDOWS */
#endif /* not USE_SIGNAL_TIMEOUT */

#ifndef WINDOWS

/* Sleep the specified amount of seconds.  On machines without
   nanosleep(), this may sleep shorter if interrupted by signals.  */

void
xsleep (double seconds)
{
#ifdef HAVE_NANOSLEEP
  /* nanosleep is the preferred interface because it offers high
     accuracy and, more importantly, because it allows us to reliably
     restart receiving a signal such as SIGWINCH.  (There was an
     actual Debian bug report about --limit-rate malfunctioning while
     the terminal was being resized.)  */
  struct timespec sleep, remaining;
  sleep.tv_sec = (long) seconds;
  sleep.tv_nsec = 1000000000 * (seconds - (long) seconds);
  while (nanosleep (&sleep, &remaining) < 0 && errno == EINTR)
    /* If nanosleep has been interrupted by a signal, adjust the
       sleeping period and return to sleep.  */
    sleep = remaining;
#else  /* not HAVE_NANOSLEEP */
#ifdef HAVE_USLEEP
  /* If usleep is available, use it in preference to select.  */
  if (seconds >= 1)
    {
      /* On some systems, usleep cannot handle values larger than
	 1,000,000.  If the period is larger than that, use sleep
	 first, then add usleep for subsecond accuracy.  */
      sleep (seconds);
      seconds -= (long) seconds;
    }
  usleep (seconds * 1000000);
#else  /* not HAVE_USLEEP */
#ifdef HAVE_SELECT
  /* Note that, although Windows supports select, this sleeping
     strategy doesn't work there because Winsock's select doesn't
     implement timeout when it is passed NULL pointers for all fd
     sets.  (But it does work under Cygwin, which implements its own
     select.)  */
  struct timeval sleep;
  sleep.tv_sec = (long) seconds;
  sleep.tv_usec = 1000000 * (seconds - (long) seconds);
  select (0, NULL, NULL, NULL, &sleep);
  /* If select returns -1 and errno is EINTR, it means we were
     interrupted by a signal.  But without knowing how long we've
     actually slept, we can't return to sleep.  Using gettimeofday to
     track sleeps is slow and unreliable due to clock skew.  */
#else  /* not HAVE_SELECT */
  sleep (seconds);
#endif /* not HAVE_SELECT */
#endif /* not HAVE_USLEEP */
#endif /* not HAVE_NANOSLEEP */
}

#endif /* not WINDOWS */

/* Encode the string STR of length LENGTH to base64 format and place it
   to B64STORE.  The output will be \0-terminated, and must point to a
   writable buffer of at least 1+BASE64_LENGTH(length) bytes.  It
   returns the length of the resulting base64 data, not counting the
   terminating zero.

   This implementation will not emit newlines after 76 characters of
   base64 data.  */

int
base64_encode (const char *str, int length, char *b64store)
{
  /* Conversion table.  */
  static char tbl[64] = {
    'A','B','C','D','E','F','G','H',
    'I','J','K','L','M','N','O','P',
    'Q','R','S','T','U','V','W','X',
    'Y','Z','a','b','c','d','e','f',
    'g','h','i','j','k','l','m','n',
    'o','p','q','r','s','t','u','v',
    'w','x','y','z','0','1','2','3',
    '4','5','6','7','8','9','+','/'
  };
  int i;
  const unsigned char *s = (const unsigned char *) str;
  char *p = b64store;

  /* Transform the 3x8 bits to 4x6 bits, as required by base64.  */
  for (i = 0; i < length; i += 3)
    {
      *p++ = tbl[s[0] >> 2];
      *p++ = tbl[((s[0] & 3) << 4) + (s[1] >> 4)];
      *p++ = tbl[((s[1] & 0xf) << 2) + (s[2] >> 6)];
      *p++ = tbl[s[2] & 0x3f];
      s += 3;
    }

  /* Pad the result if necessary...  */
  if (i == length + 1)
    *(p - 1) = '=';
  else if (i == length + 2)
    *(p - 1) = *(p - 2) = '=';

  /* ...and zero-terminate it.  */
  *p = '\0';

  return p - b64store;
}

#define IS_ASCII(c) (((c) & 0x80) == 0)
#define IS_BASE64(c) ((IS_ASCII (c) && base64_char_to_value[c] >= 0) || c == '=')

/* Get next character from the string, except that non-base64
   characters are ignored, as mandated by rfc2045.  */
#define NEXT_BASE64_CHAR(c, p) do {			\
  c = *p++;						\
} while (c != '\0' && !IS_BASE64 (c))

/* Decode data from BASE64 (assumed to be encoded as base64) into
   memory pointed to by TO.  TO should be large enough to accomodate
   the decoded data, which is guaranteed to be less than
   strlen(base64).

   Since TO is assumed to contain binary data, it is not
   NUL-terminated.  The function returns the length of the data
   written to TO.  -1 is returned in case of error caused by malformed
   base64 input.  */

int
base64_decode (const char *base64, char *to)
{
  /* Table of base64 values for first 128 characters.  Note that this
     assumes ASCII (but so does Wget in other places).  */
  static short base64_char_to_value[128] =
    {
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,	/*   0-  9 */
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,	/*  10- 19 */
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,	/*  20- 29 */
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,	/*  30- 39 */
      -1,  -1,  -1,  62,  -1,  -1,  -1,  63,  52,  53,	/*  40- 49 */
      54,  55,  56,  57,  58,  59,  60,  61,  -1,  -1,	/*  50- 59 */
      -1,  -1,  -1,  -1,  -1,  0,   1,   2,   3,   4,	/*  60- 69 */
      5,   6,   7,   8,   9,   10,  11,  12,  13,  14,	/*  70- 79 */
      15,  16,  17,  18,  19,  20,  21,  22,  23,  24,	/*  80- 89 */
      25,  -1,  -1,  -1,  -1,  -1,  -1,  26,  27,  28,	/*  90- 99 */
      29,  30,  31,  32,  33,  34,  35,  36,  37,  38,	/* 100-109 */
      39,  40,  41,  42,  43,  44,  45,  46,  47,  48,	/* 110-119 */
      49,  50,  51,  -1,  -1,  -1,  -1,  -1		/* 120-127 */
    };

  const char *p = base64;
  char *q = to;

  while (1)
    {
      unsigned char c;
      unsigned long value;

      /* Process first byte of a quadruplet.  */
      NEXT_BASE64_CHAR (c, p);
      if (!c)
	break;
      if (c == '=')
	return -1;		/* illegal '=' while decoding base64 */
      value = base64_char_to_value[c] << 18;

      /* Process scond byte of a quadruplet.  */
      NEXT_BASE64_CHAR (c, p);
      if (!c)
	return -1;		/* premature EOF while decoding base64 */
      if (c == '=')
	return -1;		/* illegal `=' while decoding base64 */
      value |= base64_char_to_value[c] << 12;
      *q++ = value >> 16;

      /* Process third byte of a quadruplet.  */
      NEXT_BASE64_CHAR (c, p);
      if (!c)
	return -1;		/* premature EOF while decoding base64 */

      if (c == '=')
	{
	  NEXT_BASE64_CHAR (c, p);
	  if (!c)
	    return -1;		/* premature EOF while decoding base64 */
	  if (c != '=')
	    return -1;		/* padding `=' expected but not found */
	  continue;
	}

      value |= base64_char_to_value[c] << 6;
      *q++ = 0xff & value >> 8;

      /* Process fourth byte of a quadruplet.  */
      NEXT_BASE64_CHAR (c, p);
      if (!c)
	return -1;		/* premature EOF while decoding base64 */
      if (c == '=')
	continue;

      value |= base64_char_to_value[c];
      *q++ = 0xff & value;
    }

  return q - to;
}

#undef IS_ASCII
#undef IS_BASE64
#undef NEXT_BASE64_CHAR

/* Simple merge sort for use by stable_sort.  Implementation courtesy
   Zeljko Vrba with additional debugging by Nenad Barbutov.  */

static void
mergesort_internal (void *base, void *temp, size_t size, size_t from, size_t to,
		    int (*cmpfun) PARAMS ((const void *, const void *)))
{
#define ELT(array, pos) ((char *)(array) + (pos) * size)
  if (from < to)
    {
      size_t i, j, k;
      size_t mid = (to + from) / 2;
      mergesort_internal (base, temp, size, from, mid, cmpfun);
      mergesort_internal (base, temp, size, mid + 1, to, cmpfun);
      i = from;
      j = mid + 1;
      for (k = from; (i <= mid) && (j <= to); k++)
	if (cmpfun (ELT (base, i), ELT (base, j)) <= 0)
	  memcpy (ELT (temp, k), ELT (base, i++), size);
	else
	  memcpy (ELT (temp, k), ELT (base, j++), size);
      while (i <= mid)
	memcpy (ELT (temp, k++), ELT (base, i++), size);
      while (j <= to)
	memcpy (ELT (temp, k++), ELT (base, j++), size);
      for (k = from; k <= to; k++)
	memcpy (ELT (base, k), ELT (temp, k), size);
    }
#undef ELT
}

/* Stable sort with interface exactly like standard library's qsort.
   Uses mergesort internally, allocating temporary storage with
   alloca.  */

void
stable_sort (void *base, size_t nmemb, size_t size,
	     int (*cmpfun) PARAMS ((const void *, const void *)))
{
  if (size > 1)
    {
      void *temp = alloca (nmemb * size * sizeof (void *));
      mergesort_internal (base, temp, size, 0, nmemb - 1, cmpfun);
    }
}
