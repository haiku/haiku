/* pathexp.c -- The shell interface to the globbing library. */

/* Copyright (C) 1995-2002 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2, or (at your option) any later
   version.

   Bash is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License along
   with Bash; see the file COPYING.  If not, write to the Free Software
   Foundation, 59 Temple Place, Suite 330, Boston, MA 02111 USA. */

#include "config.h"

#include "bashtypes.h"
#include <stdio.h>

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include "bashansi.h"

#include "shell.h"
#include "pathexp.h"
#include "flags.h"

#include "shmbutil.h"

#include <glob/strmatch.h>

static int glob_name_is_acceptable __P((const char *));
static void ignore_globbed_names __P((char **, sh_ignore_func_t *));
               
#if defined (USE_POSIX_GLOB_LIBRARY)
#  include <glob.h>
typedef int posix_glob_errfunc_t __P((const char *, int));
#else
#  include <glob/glob.h>
#endif

/* Control whether * matches .files in globbing. */
int glob_dot_filenames;

/* Control whether the extended globbing features are enabled. */
int extended_glob = 0;

/* Return nonzero if STRING has any unquoted special globbing chars in it.  */
int
unquoted_glob_pattern_p (string)
     register char *string;
{
  register int c;
  char *send;
  int open;

  DECLARE_MBSTATE;

  open = 0;
  send = string + strlen (string);

  while (c = *string++)
    {
      switch (c)
	{
	case '?':
	case '*':
	  return (1);

	case '[':
	  open++;
	  continue;

	case ']':
	  if (open)
	    return (1);
	  continue;

	case '+':
	case '@':
	case '!':
	  if (*string == '(')	/*)*/
	    return (1);
	  continue;

	case CTLESC:
	case '\\':
	  if (*string++ == '\0')
	    return (0);
	}

      /* Advance one fewer byte than an entire multibyte character to
	 account for the auto-increment in the loop above. */
#ifdef HANDLE_MULTIBYTE
      string--;
      ADVANCE_CHAR_P (string, send - string);
      string++;
#else
      ADVANCE_CHAR_P (string, send - string);
#endif
    }
  return (0);
}

/* PATHNAME can contain characters prefixed by CTLESC; this indicates
   that the character is to be quoted.  We quote it here in the style
   that the glob library recognizes.  If flags includes QGLOB_CVTNULL,
   we change quoted null strings (pathname[0] == CTLNUL) into empty
   strings (pathname[0] == 0).  If this is called after quote removal
   is performed, (flags & QGLOB_CVTNULL) should be 0; if called when quote
   removal has not been done (for example, before attempting to match a
   pattern while executing a case statement), flags should include
   QGLOB_CVTNULL.  If flags includes QGLOB_FILENAME, appropriate quoting
   to match a filename should be performed. */
char *
quote_string_for_globbing (pathname, qflags)
     const char *pathname;
     int qflags;
{
  char *temp;
  register int i, j;

  temp = (char *)xmalloc (strlen (pathname) + 1);

  if ((qflags & QGLOB_CVTNULL) && QUOTED_NULL (pathname))
    {
      temp[0] = '\0';
      return temp;
    }

  for (i = j = 0; pathname[i]; i++)
    {
      if (pathname[i] == CTLESC)
	{
	  if ((qflags & QGLOB_FILENAME) && pathname[i+1] == '/')
	    continue;
	  temp[j++] = '\\';
	  i++;
	  if (pathname[i] == '\0')
	    break;
	}
      temp[j++] = pathname[i];
    }
  temp[j] = '\0';

  return (temp);
}

char *
quote_globbing_chars (string)
     char *string;
{
  size_t slen;
  char *temp, *s, *t, *send;
  DECLARE_MBSTATE;

  slen = strlen (string);
  send = string + slen;

  temp = (char *)xmalloc (slen * 2 + 1);
  for (t = temp, s = string; *s; )
    {
      switch (*s)
	{
	case '*':
	case '[':
	case ']':
	case '?':
	case '\\':
	  *t++ = '\\';
	  break;
	case '+':
	case '@':
	case '!':
	  if (s[1] == '(')	/*(*/
	    *t++ = '\\';
	  break;
	}

      /* Copy a single (possibly multibyte) character from s to t,
         incrementing both. */
      COPY_CHAR_P (t, s, send);
    }
  *t = '\0';
  return temp;
}

/* Call the glob library to do globbing on PATHNAME. */
char **
shell_glob_filename (pathname)
     const char *pathname;
{
#if defined (USE_POSIX_GLOB_LIBRARY)
  register int i;
  char *temp, **results;
  glob_t filenames;
  int glob_flags;

  temp = quote_string_for_globbing (pathname, QGLOB_FILENAME);

  filenames.gl_offs = 0;

#  if defined (GLOB_PERIOD)
  glob_flags = glob_dot_filenames ? GLOB_PERIOD : 0;
#  else
  glob_flags = 0;
#  endif /* !GLOB_PERIOD */

  glob_flags |= (GLOB_ERR | GLOB_DOOFFS);

  i = glob (temp, glob_flags, (posix_glob_errfunc_t *)NULL, &filenames);

  free (temp);

  if (i == GLOB_NOSPACE || i == GLOB_ABORTED)
    return ((char **)NULL);
  else if (i == GLOB_NOMATCH)
    filenames.gl_pathv = (char **)NULL;
  else if (i != 0)		/* other error codes not in POSIX.2 */
    filenames.gl_pathv = (char **)NULL;

  results = filenames.gl_pathv;

  if (results && ((GLOB_FAILED (results)) == 0))
    {
      if (should_ignore_glob_matches ())
	ignore_glob_matches (results);
      if (results && results[0])
	strvec_sort (results);
      else
	{
	  FREE (results);
	  results = (char **)NULL;
	}
    }

  return (results);

#else /* !USE_POSIX_GLOB_LIBRARY */

  char *temp, **results;

  noglob_dot_filenames = glob_dot_filenames == 0;

  temp = quote_string_for_globbing (pathname, QGLOB_FILENAME);
  results = glob_filename (temp, 0);
  free (temp);

  if (results && ((GLOB_FAILED (results)) == 0))
    {
      if (should_ignore_glob_matches ())
	ignore_glob_matches (results);
      if (results && results[0])
	strvec_sort (results);
      else
	{
	  FREE (results);
	  results = (char **)&glob_error_return;
	}
    }

  return (results);
#endif /* !USE_POSIX_GLOB_LIBRARY */
}

/* Stuff for GLOBIGNORE. */

static struct ignorevar globignore =
{
  "GLOBIGNORE",
  (struct ign *)0,
  0,
  (char *)0,
  (sh_iv_item_func_t *)0,
};

/* Set up to ignore some glob matches because the value of GLOBIGNORE
   has changed.  If GLOBIGNORE is being unset, we also need to disable
   the globbing of filenames beginning with a `.'. */
void
setup_glob_ignore (name)
     char *name;
{
  char *v;

  v = get_string_value (name);
  setup_ignore_patterns (&globignore);

  if (globignore.num_ignores)
    glob_dot_filenames = 1;
  else if (v == 0)
    glob_dot_filenames = 0;
}

int
should_ignore_glob_matches ()
{
  return globignore.num_ignores;
}

/* Return 0 if NAME matches a pattern in the globignore.ignores list. */
static int
glob_name_is_acceptable (name)
     const char *name;
{
  struct ign *p;
  int flags;

  /* . and .. are never matched */
  if (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')))
    return (0);

  flags = FNM_PATHNAME | FNMATCH_EXTFLAG;
  for (p = globignore.ignores; p->val; p++)
    {
      if (strmatch (p->val, (char *)name, flags) != FNM_NOMATCH)
	return (0);
    }
  return (1);
}

/* Internal function to test whether filenames in NAMES should be
   ignored.  NAME_FUNC is a pointer to a function to call with each
   name.  It returns non-zero if the name is acceptable to the particular
   ignore function which called _ignore_names; zero if the name should
   be removed from NAMES. */

static void
ignore_globbed_names (names, name_func)
     char **names;
     sh_ignore_func_t *name_func;
{
  char **newnames;
  int n, i;

  for (i = 0; names[i]; i++)
    ;
  newnames = strvec_create (i + 1);

  for (n = i = 0; names[i]; i++)
    {
      if ((*name_func) (names[i]))
	newnames[n++] = names[i];
      else
	free (names[i]);
    }

  newnames[n] = (char *)NULL;

  if (n == 0)
    {
      names[0] = (char *)NULL;
      free (newnames);
      return;
    }

  /* Copy the acceptable names from NEWNAMES back to NAMES and set the
     new array end. */
  for (n = 0; newnames[n]; n++)
    names[n] = newnames[n];
  names[n] = (char *)NULL;
  free (newnames);
}

void
ignore_glob_matches (names)
     char **names;
{
  if (globignore.num_ignores == 0)
    return;

  ignore_globbed_names (names, glob_name_is_acceptable);
}

void
setup_ignore_patterns (ivp)
     struct ignorevar *ivp;
{
  int numitems, maxitems, ptr;
  char *colon_bit, *this_ignoreval;
  struct ign *p;

  this_ignoreval = get_string_value (ivp->varname);

  /* If nothing has changed then just exit now. */
  if ((this_ignoreval && ivp->last_ignoreval && STREQ (this_ignoreval, ivp->last_ignoreval)) ||
      (!this_ignoreval && !ivp->last_ignoreval))
    return;

  /* Oops.  The ignore variable has changed.  Re-parse it. */
  ivp->num_ignores = 0;

  if (ivp->ignores)
    {
      for (p = ivp->ignores; p->val; p++)
	free(p->val);
      free (ivp->ignores);
      ivp->ignores = (struct ign *)NULL;
    }

  if (ivp->last_ignoreval)
    {
      free (ivp->last_ignoreval);
      ivp->last_ignoreval = (char *)NULL;
    }

  if (this_ignoreval == 0 || *this_ignoreval == '\0')
    return;

  ivp->last_ignoreval = savestring (this_ignoreval);

  numitems = maxitems = ptr = 0;

  while (colon_bit = extract_colon_unit (this_ignoreval, &ptr))
    {
      if (numitems + 1 >= maxitems)
	{
	  maxitems += 10;
	  ivp->ignores = (struct ign *)xrealloc (ivp->ignores, maxitems * sizeof (struct ign));
	}
      ivp->ignores[numitems].val = colon_bit;
      ivp->ignores[numitems].len = strlen (colon_bit);
      ivp->ignores[numitems].flags = 0;
      if (ivp->item_func)
	(*ivp->item_func) (&ivp->ignores[numitems]);
      numitems++;
    }
  ivp->ignores[numitems].val = (char *)NULL;
  ivp->num_ignores = numitems;
}
