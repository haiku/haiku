/* glob.c -- file-name wildcard pattern matching for Bash.

   Copyright (C) 1985-2002 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111 USA.  */

/* To whomever it may concern: I have never seen the code which most
   Unix programs use to perform this function.  I wrote this from scratch
   based on specifications for the pattern matching.  --RMS.  */

#include <config.h>

#if !defined (__GNUC__) && !defined (HAVE_ALLOCA_H) && defined (_AIX)
  #pragma alloca
#endif /* _AIX && RISC6000 && !__GNUC__ */

#include "bashtypes.h"

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include "bashansi.h"
#include "posixdir.h"
#include "posixstat.h"
#include "shmbutil.h"
#include "xmalloc.h"

#include "filecntl.h"
#if !defined (F_OK)
#  define F_OK 0
#endif

#include "stdc.h"
#include "memalloc.h"
#include "quit.h"

#include "glob.h"
#include "strmatch.h"

#if !defined (HAVE_BCOPY) && !defined (bcopy)
#  define bcopy(s, d, n) ((void) memcpy ((d), (s), (n)))
#endif /* !HAVE_BCOPY && !bcopy */

#if !defined (NULL)
#  if defined (__STDC__)
#    define NULL ((void *) 0)
#  else
#    define NULL 0x0
#  endif /* __STDC__ */
#endif /* !NULL */

extern void throw_to_top_level __P((void));
extern int test_eaccess __P((char *, int));

extern int extended_glob;

/* Global variable which controls whether or not * matches .*.
   Non-zero means don't match .*.  */
int noglob_dot_filenames = 1;

/* Global variable which controls whether or not filename globbing
   is done without regard to case. */
int glob_ignore_case = 0;

/* Global variable to return to signify an error in globbing. */
char *glob_error_return;

/* Some forward declarations. */
static int skipname __P((char *, char *));
#if HANDLE_MULTIBYTE
static int mbskipname __P((char *, char *));
#endif
#if HANDLE_MULTIBYTE
static void udequote_pathname __P((char *));
static void wdequote_pathname __P((char *));
#else
#  define dequote_pathname udequote_pathname
#endif
static void dequote_pathname __P((char *));
static int glob_testdir __P((char *));
static char **glob_dir_to_array __P((char *, char **, int));

/* Compile `glob_loop.c' for single-byte characters. */
#define CHAR	unsigned char
#define INT	int
#define L(CS)	CS
#define INTERNAL_GLOB_PATTERN_P internal_glob_pattern_p
#include "glob_loop.c"

/* Compile `glob_loop.c' again for multibyte characters. */
#if HANDLE_MULTIBYTE

#define CHAR	wchar_t
#define INT	wint_t
#define L(CS)	L##CS
#define INTERNAL_GLOB_PATTERN_P internal_glob_wpattern_p
#include "glob_loop.c"

#endif /* HANDLE_MULTIBYTE */

/* And now a function that calls either the single-byte or multibyte version
   of internal_glob_pattern_p. */
int
glob_pattern_p (pattern)
     const char *pattern;
{
#if HANDLE_MULTIBYTE
  mbstate_t ps;
  size_t n;
  wchar_t *wpattern;
  int r;

  if (MB_CUR_MAX == 1)
    return (internal_glob_pattern_p (pattern));

  /* Convert strings to wide chars, and call the multibyte version. */
  memset (&ps, '\0', sizeof (ps));
  n = xmbsrtowcs (NULL, (const char **)&pattern, 0, &ps);
  if (n == (size_t)-1)
    /* Oops.  Invalid multibyte sequence.  Try it as single-byte sequence. */
    return (internal_glob_pattern_p (pattern));
  wpattern = (wchar_t *)xmalloc ((n + 1) * sizeof (wchar_t));
  (void) xmbsrtowcs (wpattern, (const char **)&pattern, n + 1, &ps);
  r = internal_glob_wpattern_p (wpattern);
  free (wpattern);
  return r;
#else
  return (internal_glob_pattern_p (pattern));
#endif
}

/* Return 1 if DNAME should be skipped according to PAT.  Mostly concerned
   with matching leading `.'. */

static int
skipname (pat, dname)
     char *pat;
     char *dname;
{
  /* If a leading dot need not be explicitly matched, and the pattern
     doesn't start with a `.', don't match `.' or `..' */
  if (noglob_dot_filenames == 0 && pat[0] != '.' &&
	(pat[0] != '\\' || pat[1] != '.') &&
	(dname[0] == '.' &&
	  (dname[1] == '\0' || (dname[1] == '.' && dname[2] == '\0'))))
    return 1;

  /* If a dot must be explicity matched, check to see if they do. */
  else if (noglob_dot_filenames && dname[0] == '.' && pat[0] != '.' &&
	(pat[0] != '\\' || pat[1] != '.'))
    return 1;

  return 0;
}

#if HANDLE_MULTIBYTE
/* Return 1 if DNAME should be skipped according to PAT.  Handles multibyte
   characters in PAT and DNAME.  Mostly concerned with matching leading `.'. */

static int
mbskipname (pat, dname)
     char *pat, *dname;
{
  char *pat_bak, *dn_bak;
  wchar_t *pat_wc, *dn_wc;
  mbstate_t pat_ps, dn_ps;
  size_t pat_n, dn_n, n;

  n = strlen(pat);
  pat_bak = (char *) alloca (n + 1);
  memcpy (pat_bak, pat, n + 1);

  n = strlen(dname);
  dn_bak = (char *) alloca (n + 1);
  memcpy (dn_bak, dname,  n + 1);

  memset(&pat_ps, '\0', sizeof(mbstate_t));
  memset(&dn_ps, '\0', sizeof(mbstate_t));

  pat_n = xmbsrtowcs (NULL, (const char **)&pat_bak, 0, &pat_ps);
  dn_n = xmbsrtowcs (NULL, (const char **)&dn_bak, 0, &dn_ps);

  if (pat_n != (size_t)-1 && dn_n !=(size_t)-1)
    {
      pat_wc = (wchar_t *) alloca ((pat_n + 1) * sizeof(wchar_t));
      dn_wc = (wchar_t *) alloca ((dn_n + 1) * sizeof(wchar_t));

      (void) xmbsrtowcs (pat_wc, (const char **)&pat_bak, pat_n + 1, &pat_ps);
      (void) xmbsrtowcs (dn_wc, (const char **)&dn_bak, dn_n + 1, &dn_ps);

      /* If a leading dot need not be explicitly matched, and the
	 pattern doesn't start with a `.', don't match `.' or `..' */
      if (noglob_dot_filenames == 0 && pat_wc[0] != L'.' &&
	    (pat_wc[0] != L'\\' || pat_wc[1] != L'.') &&
	    (dn_wc[0] == L'.' &&
	      (dn_wc[1] == L'\0' || (dn_wc[1] == L'.' && dn_wc[2] == L'\0'))))
	return 1;

      /* If a leading dot must be explicity matched, check to see if the
	 pattern and dirname both have one. */
     else if (noglob_dot_filenames && dn_wc[0] == L'.' &&
	   pat_wc[0] != L'.' &&
	   (pat_wc[0] != L'\\' || pat_wc[1] != L'.'))
	return 1;
    }

  return 0;
}
#endif /* HANDLE_MULTIBYTE */

/* Remove backslashes quoting characters in PATHNAME by modifying PATHNAME. */
static void
udequote_pathname (pathname)
     char *pathname;
{
  register int i, j;

  for (i = j = 0; pathname && pathname[i]; )
    {
      if (pathname[i] == '\\')
	i++;

      pathname[j++] = pathname[i++];

      if (!pathname[i - 1])
	break;
    }
  pathname[j] = '\0';
}

#if HANDLE_MULTIBYTE
/* Remove backslashes quoting characters in PATHNAME by modifying PATHNAME. */
static void
wdequote_pathname (pathname)
     char *pathname;
{
  mbstate_t ps;
  size_t len, n;
  wchar_t *wpathname;
  char *pathname_bak;
  int i, j;

  len = strlen (pathname);
  pathname_bak = (char *) alloca (len + 1);
  memcpy (pathname_bak, pathname , len + 1);

  /* Convert the strings into wide characters.  */
  memset (&ps, '\0', sizeof (ps));
  n = xmbsrtowcs (NULL, (const char **)&pathname_bak, 0, &ps);
  if (n == (size_t) -1)
    /* Something wrong. */
    return;

  wpathname = (wchar_t *) alloca ((n + 1) * sizeof (wchar_t));
  (void) xmbsrtowcs (wpathname, (const char **)&pathname_bak, n + 1, &ps);

  for (i = j = 0; wpathname && wpathname[i]; )
    {
      if (wpathname[i] == L'\\')
	i++;

      wpathname[j++] = wpathname[i++];

      if (!wpathname[i - 1])
	break;
    }
  wpathname[j] = L'\0';

  /* Convert the wide character string into unibyte character set. */
  memset (&ps, '\0', sizeof(mbstate_t));
  n = wcsrtombs(pathname, (const wchar_t **)&wpathname, len, &ps);
  pathname[len] = '\0';
}

static void
dequote_pathname (pathname)
     char *pathname;
{
  if (MB_CUR_MAX > 1)
    wdequote_pathname (pathname);
  else
    udequote_pathname (pathname);
}
#endif /* HANDLE_MULTIBYTE */

/* Test whether NAME exists. */

#if defined (HAVE_LSTAT)
#  define GLOB_TESTNAME(name)  (lstat (name, &finfo))
#else /* !HAVE_LSTAT */
#  if !defined (AFS)
#    define GLOB_TESTNAME(name)  (test_eaccess (nextname, F_OK))
#  else /* AFS */
#    define GLOB_TESTNAME(name)  (access (nextname, F_OK))
#  endif /* AFS */
#endif /* !HAVE_LSTAT */

/* Return 0 if DIR is a directory, -1 otherwise. */
static int
glob_testdir (dir)
     char *dir;
{
  struct stat finfo;

  if (stat (dir, &finfo) < 0)
    return (-1);

  if (S_ISDIR (finfo.st_mode) == 0)
    return (-1);

  return (0);
}

/* Return a vector of names of files in directory DIR
   whose names match glob pattern PAT.
   The names are not in any particular order.
   Wildcards at the beginning of PAT do not match an initial period.

   The vector is terminated by an element that is a null pointer.

   To free the space allocated, first free the vector's elements,
   then free the vector.

   Return 0 if cannot get enough memory to hold the pointer
   and the names.

   Return -1 if cannot access directory DIR.
   Look in errno for more information.  */

char **
glob_vector (pat, dir, flags)
     char *pat;
     char *dir;
     int flags;
{
  struct globval
    {
      struct globval *next;
      char *name;
    };

  DIR *d;
  register struct dirent *dp;
  struct globval *lastlink;
  register struct globval *nextlink;
  register char *nextname, *npat;
  unsigned int count;
  int lose, skip;
  register char **name_vector;
  register unsigned int i;
  int mflags;		/* Flags passed to strmatch (). */

  lastlink = 0;
  count = lose = skip = 0;

  /* If PAT is empty, skip the loop, but return one (empty) filename. */
  if (pat == 0 || *pat == '\0')
    {
      if (glob_testdir (dir) < 0)
	return ((char **) &glob_error_return);

      nextlink = (struct globval *)alloca (sizeof (struct globval));
      nextlink->next = (struct globval *)0;
      nextname = (char *) malloc (1);
      if (nextname == 0)
	lose = 1;
      else
	{
	  lastlink = nextlink;
	  nextlink->name = nextname;
	  nextname[0] = '\0';
	  count = 1;
	}

      skip = 1;
    }

  /* If the filename pattern (PAT) does not contain any globbing characters,
     we can dispense with reading the directory, and just see if there is
     a filename `DIR/PAT'.  If there is, and we can access it, just make the
     vector to return and bail immediately. */
  if (skip == 0 && glob_pattern_p (pat) == 0)
    {
      int dirlen;
      struct stat finfo;

      if (glob_testdir (dir) < 0)
	return ((char **) &glob_error_return);

      dirlen = strlen (dir);
      nextname = (char *)malloc (dirlen + strlen (pat) + 2);
      npat = (char *)malloc (strlen (pat) + 1);
      if (nextname == 0 || npat == 0)
	lose = 1;
      else
	{
	  strcpy (npat, pat);
	  dequote_pathname (npat);

	  strcpy (nextname, dir);
	  nextname[dirlen++] = '/';
	  strcpy (nextname + dirlen, npat);

	  if (GLOB_TESTNAME (nextname) >= 0)
	    {
	      free (nextname);
	      nextlink = (struct globval *)alloca (sizeof (struct globval));
	      nextlink->next = (struct globval *)0;
	      lastlink = nextlink;
	      nextlink->name = npat;
	      count = 1;
	    }
	  else
	    {
	      free (nextname);
	      free (npat);
	    }
	}

      skip = 1;
    }

  if (skip == 0)
    {
      /* Open the directory, punting immediately if we cannot.  If opendir
	 is not robust (i.e., it opens non-directories successfully), test
	 that DIR is a directory and punt if it's not. */
#if defined (OPENDIR_NOT_ROBUST)
      if (glob_testdir (dir) < 0)
	return ((char **) &glob_error_return);
#endif

      d = opendir (dir);
      if (d == NULL)
	return ((char **) &glob_error_return);

      /* Compute the flags that will be passed to strmatch().  We don't
	 need to do this every time through the loop. */
      mflags = (noglob_dot_filenames ? FNM_PERIOD : 0) | FNM_PATHNAME;

#ifdef FNM_CASEFOLD
      if (glob_ignore_case)
	mflags |= FNM_CASEFOLD;
#endif

      if (extended_glob)
	mflags |= FNM_EXTMATCH;

      /* Scan the directory, finding all names that match.
	 For each name that matches, allocate a struct globval
	 on the stack and store the name in it.
	 Chain those structs together; lastlink is the front of the chain.  */
      while (1)
	{
	  /* Make globbing interruptible in the shell. */
	  if (interrupt_state)
	    {
	      lose = 1;
	      break;
	    }
	  
	  dp = readdir (d);
	  if (dp == NULL)
	    break;

	  /* If this directory entry is not to be used, try again. */
	  if (REAL_DIR_ENTRY (dp) == 0)
	    continue;

#if HANDLE_MULTIBYTE
	  if (MB_CUR_MAX > 1 && mbskipname (pat, dp->d_name))
	    continue;
	  else
#endif
	  if (skipname (pat, dp->d_name))
	    continue;

	  if (strmatch (pat, dp->d_name, mflags) != FNM_NOMATCH)
	    {
	      nextlink = (struct globval *) alloca (sizeof (struct globval));
	      nextlink->next = lastlink;
	      nextname = (char *) malloc (D_NAMLEN (dp) + 1);
	      if (nextname == NULL)
		{
		  lose = 1;
		  break;
		}
	      lastlink = nextlink;
	      nextlink->name = nextname;
	      bcopy (dp->d_name, nextname, D_NAMLEN (dp) + 1);
	      ++count;
	    }
	}

      (void) closedir (d);
    }

  if (lose == 0)
    {
      name_vector = (char **) malloc ((count + 1) * sizeof (char *));
      lose |= name_vector == NULL;
    }

  /* Have we run out of memory?	 */
  if (lose)
    {
      /* Here free the strings we have got.  */
      while (lastlink)
	{
	  free (lastlink->name);
	  lastlink = lastlink->next;
	}

      QUIT;

      return ((char **)NULL);
    }

  /* Copy the name pointers from the linked list into the vector.  */
  for (i = 0; i < count; ++i)
    {
      name_vector[i] = lastlink->name;
      lastlink = lastlink->next;
    }

  name_vector[count] = NULL;
  return (name_vector);
}

/* Return a new array which is the concatenation of each string in ARRAY
   to DIR.  This function expects you to pass in an allocated ARRAY, and
   it takes care of free()ing that array.  Thus, you might think of this
   function as side-effecting ARRAY.  This should handle GX_MARKDIRS. */
static char **
glob_dir_to_array (dir, array, flags)
     char *dir, **array;
     int flags;
{
  register unsigned int i, l;
  int add_slash;
  char **result, *new;
  struct stat sb;

  l = strlen (dir);
  if (l == 0)
    {
      if (flags & GX_MARKDIRS)
	for (i = 0; array[i]; i++)
	  {
	    if ((stat (array[i], &sb) == 0) && S_ISDIR (sb.st_mode))
	      {
		l = strlen (array[i]);
		new = (char *)realloc (array[i], l + 2);
		if (new == 0)
		  return NULL;
		new[l] = '/';
		new[l+1] = '\0';
		array[i] = new;
	      }
	  }
      return (array);
    }

  add_slash = dir[l - 1] != '/';

  i = 0;
  while (array[i] != NULL)
    ++i;

  result = (char **) malloc ((i + 1) * sizeof (char *));
  if (result == NULL)
    return (NULL);

  for (i = 0; array[i] != NULL; i++)
    {
      /* 3 == 1 for NUL, 1 for slash at end of DIR, 1 for GX_MARKDIRS */
      result[i] = (char *) malloc (l + strlen (array[i]) + 3);

      if (result[i] == NULL)
	return (NULL);

      strcpy (result[i], dir);
      if (add_slash)
	result[i][l] = '/';
      strcpy (result[i] + l + add_slash, array[i]);
      if (flags & GX_MARKDIRS)
	{
	  if ((stat (result[i], &sb) == 0) && S_ISDIR (sb.st_mode))
	    {
	      size_t rlen;
	      rlen = strlen (result[i]);
	      result[i][rlen] = '/';
	      result[i][rlen+1] = '\0';
	    }
	}
    }
  result[i] = NULL;

  /* Free the input array.  */
  for (i = 0; array[i] != NULL; i++)
    free (array[i]);
  free ((char *) array);

  return (result);
}

/* Do globbing on PATHNAME.  Return an array of pathnames that match,
   marking the end of the array with a null-pointer as an element.
   If no pathnames match, then the array is empty (first element is null).
   If there isn't enough memory, then return NULL.
   If a file system error occurs, return -1; `errno' has the error code.  */
char **
glob_filename (pathname, flags)
     char *pathname;
     int flags;
{
  char **result;
  unsigned int result_size;
  char *directory_name, *filename;
  unsigned int directory_len;

  result = (char **) malloc (sizeof (char *));
  result_size = 1;
  if (result == NULL)
    return (NULL);

  result[0] = NULL;

  /* Find the filename.  */
  filename = strrchr (pathname, '/');
  if (filename == NULL)
    {
      filename = pathname;
      directory_name = "";
      directory_len = 0;
    }
  else
    {
      directory_len = (filename - pathname) + 1;
      directory_name = (char *) alloca (directory_len + 1);

      bcopy (pathname, directory_name, directory_len);
      directory_name[directory_len] = '\0';
      ++filename;
    }

  /* If directory_name contains globbing characters, then we
     have to expand the previous levels.  Just recurse. */
  if (glob_pattern_p (directory_name))
    {
      char **directories;
      register unsigned int i;

      if (directory_name[directory_len - 1] == '/')
	directory_name[directory_len - 1] = '\0';

      directories = glob_filename (directory_name, flags & ~GX_MARKDIRS);

      if (directories == NULL)
	goto memory_error;
      else if (directories == (char **)&glob_error_return)
	{
	  free ((char *) result);
	  return ((char **) &glob_error_return);
	}
      else if (*directories == NULL)
	{
	  free ((char *) directories);
	  free ((char *) result);
	  return ((char **) &glob_error_return);
	}

      /* We have successfully globbed the preceding directory name.
	 For each name in DIRECTORIES, call glob_vector on it and
	 FILENAME.  Concatenate the results together.  */
      for (i = 0; directories[i] != NULL; ++i)
	{
	  char **temp_results;

	  /* Scan directory even on a NULL pathname.  That way, `*h/'
	     returns only directories ending in `h', instead of all
	     files ending in `h' with a `/' appended. */
	  temp_results = glob_vector (filename, directories[i], flags & ~GX_MARKDIRS);

	  /* Handle error cases. */
	  if (temp_results == NULL)
	    goto memory_error;
	  else if (temp_results == (char **)&glob_error_return)
	    /* This filename is probably not a directory.  Ignore it.  */
	    ;
	  else
	    {
	      char **array;
	      register unsigned int l;

	      array = glob_dir_to_array (directories[i], temp_results, flags);
	      l = 0;
	      while (array[l] != NULL)
		++l;

	      result =
		(char **)realloc (result, (result_size + l) * sizeof (char *));

	      if (result == NULL)
		goto memory_error;

	      for (l = 0; array[l] != NULL; ++l)
		result[result_size++ - 1] = array[l];

	      result[result_size - 1] = NULL;

	      /* Note that the elements of ARRAY are not freed.  */
	      free ((char *) array);
	    }
	}
      /* Free the directories.  */
      for (i = 0; directories[i]; i++)
	free (directories[i]);

      free ((char *) directories);

      return (result);
    }

  /* If there is only a directory name, return it. */
  if (*filename == '\0')
    {
      result = (char **) realloc ((char *) result, 2 * sizeof (char *));
      if (result == NULL)
	return (NULL);
      /* Handle GX_MARKDIRS here. */
      result[0] = (char *) malloc (directory_len + 1);
      if (result[0] == NULL)
	goto memory_error;
      bcopy (directory_name, result[0], directory_len + 1);
      result[1] = NULL;
      return (result);
    }
  else
    {
      char **temp_results;

      /* There are no unquoted globbing characters in DIRECTORY_NAME.
	 Dequote it before we try to open the directory since there may
	 be quoted globbing characters which should be treated verbatim. */
      if (directory_len > 0)
	dequote_pathname (directory_name);

      /* We allocated a small array called RESULT, which we won't be using.
	 Free that memory now. */
      free (result);

      /* Just return what glob_vector () returns appended to the
	 directory name. */
      temp_results = glob_vector (filename,
				  (directory_len == 0 ? "." : directory_name),
				  flags & ~GX_MARKDIRS);

      if (temp_results == NULL || temp_results == (char **)&glob_error_return)
	return (temp_results);

      return (glob_dir_to_array (directory_name, temp_results, flags));
    }

  /* We get to memory_error if the program has run out of memory, or
     if this is the shell, and we have been interrupted. */
 memory_error:
  if (result != NULL)
    {
      register unsigned int i;
      for (i = 0; result[i] != NULL; ++i)
	free (result[i]);
      free ((char *) result);
    }

  QUIT;

  return (NULL);
}

#if defined (TEST)

main (argc, argv)
     int argc;
     char **argv;
{
  unsigned int i;

  for (i = 1; i < argc; ++i)
    {
      char **value = glob_filename (argv[i], 0);
      if (value == NULL)
	puts ("Out of memory.");
      else if (value == &glob_error_return)
	perror (argv[i]);
      else
	for (i = 0; value[i] != NULL; i++)
	  puts (value[i]);
    }

  exit (0);
}
#endif	/* TEST.  */
