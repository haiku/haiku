/* braces.c -- code for doing word expansion in curly braces. */

/* Copyright (C) 1987-2002 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   Bash is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash; see the file COPYING.  If not, write to the Free
   Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111 USA. */

/* Stuff in curly braces gets expanded before all other shell expansions. */

#include "config.h"

#if defined (BRACE_EXPANSION)

#if defined (HAVE_UNISTD_H)
#  ifdef _MINIX
#    include <sys/types.h>
#  endif
#  include <unistd.h>
#endif

#include "bashansi.h"

#if defined (SHELL)
#  include "shell.h"
#endif /* SHELL */

#include "general.h"
#include "shmbutil.h"

#define brace_whitespace(c) (!(c) || (c) == ' ' || (c) == '\t' || (c) == '\n')

/* Basic idea:

   Segregate the text into 3 sections: preamble (stuff before an open brace),
   postamble (stuff after the matching close brace) and amble (stuff after
   preamble, and before postamble).  Expand amble, and then tack on the
   expansions to preamble.  Expand postamble, and tack on the expansions to
   the result so far.
 */

/* The character which is used to separate arguments. */
int brace_arg_separator = ',';

#if defined (__P)
static int brace_gobbler __P((char *, size_t, int *, int));
static char **expand_amble __P((char *, size_t));
static char **array_concat __P((char **, char **));
#else
static int brace_gobbler ();
static char **expand_amble ();
static char **array_concat ();
#endif

/* Return an array of strings; the brace expansion of TEXT. */
char **
brace_expand (text)
     char *text;
{
  register int start;
  size_t tlen;
  char *preamble, *postamble, *amble;
  size_t alen;
  char **tack, **result;
  int i, j, c;

  DECLARE_MBSTATE;

  /* Find the text of the preamble. */
  tlen = strlen (text);
  i = 0;
  c = brace_gobbler (text, tlen, &i, '{');

  preamble = (char *)xmalloc (i + 1);
  strncpy (preamble, text, i);
  preamble[i] = '\0';

  result = (char **)xmalloc (2 * sizeof (char *));
  result[0] = preamble;
  result[1] = (char *)NULL;

  /* Special case.  If we never found an exciting character, then
     the preamble is all of the text, so just return that. */
  if (c != '{')
    return (result);

  /* Find the amble.  This is the stuff inside this set of braces. */
  start = ++i;
  c = brace_gobbler (text, tlen, &i, '}');

  /* What if there isn't a matching close brace? */
  if (c == 0)
    {
#if defined (NOTDEF)
      /* Well, if we found an unquoted BRACE_ARG_SEPARATOR between START
	 and I, then this should be an error.  Otherwise, it isn't. */
      j = start;
      while (j < i)
	{
	  if (text[j] == '\\')
	    {
	      j++;
	      ADVANCE_CHAR (text, tlen, j);
	      continue;
	    }

	  if (text[j] == brace_arg_separator)
	    {
	      strvec_dispose (result);
	      report_error ("missing `}'");
	      throw_to_top_level ();
	    }
	  ADVANCE_CHAR (text, tlen, j);
	}
#endif
      free (preamble);		/* Same as result[0]; see initialization. */
      result[0] = savestring (text);
      return (result);
    }

#if defined (SHELL)
  amble = substring (text, start, i);
  alen = i - start;
#else
  amble = (char *)xmalloc (1 + (i - start));
  strncpy (amble, &text[start], (i - start));
  alen = i - start;
  amble[alen] = '\0';
#endif

#if defined (SHELL)
  INITIALIZE_MBSTATE;

  /* If the amble does not contain an unquoted BRACE_ARG_SEPARATOR, then
     just return without doing any expansion.  */
  j = 0;
  while (amble[j])
    {
      if (amble[j] == '\\')
	{
	  j++;
	  ADVANCE_CHAR (amble, alen, j);
	  continue;
	}

      if (amble[j] == brace_arg_separator)
	break;

      ADVANCE_CHAR (amble, alen, j);
    }

  if (!amble[j])
    {
      free (amble);
      free (preamble);
      result[0] = savestring (text);
      return (result);
    }
#endif /* SHELL */

  postamble = &text[i + 1];

  tack = expand_amble (amble, alen);
  result = array_concat (result, tack);
  free (amble);
  strvec_dispose (tack);

  tack = brace_expand (postamble);
  result = array_concat (result, tack);
  strvec_dispose (tack);

  return (result);
}

/* Expand the text found inside of braces.  We simply try to split the
   text at BRACE_ARG_SEPARATORs into separate strings.  We then brace
   expand each slot which needs it, until there are no more slots which
   need it. */
static char **
expand_amble (text, tlen)
     char *text;
     size_t tlen;
{
  char **result, **partial;
  char *tem;
  int start, i, c;

  DECLARE_MBSTATE;

  result = (char **)NULL;

  start = i = 0;
  c = 1;
  while (c)
    {
      c = brace_gobbler (text, tlen, &i, brace_arg_separator);
#if defined (SHELL)
      tem = substring (text, start, i);
#else
      tem = (char *)xmalloc (1 + (i - start));
      strncpy (tem, &text[start], (i - start));
      tem[i- start] = '\0';
#endif

      partial = brace_expand (tem);

      if (!result)
	result = partial;
      else
	{
	  register int lr = strvec_len (result);
	  register int lp = strvec_len (partial);
	  register int j;

	  result = strvec_resize (result, lp + lr + 1);

	  for (j = 0; j < lp; j++)
	    result[lr + j] = partial[j];

	  result[lr + j] = (char *)NULL;
	  free (partial);
	}
      free (tem);
      ADVANCE_CHAR (text, tlen, i);
      start = i;
    }
  return (result);
}

/* Start at INDEX, and skip characters in TEXT. Set INDEX to the
   index of the character matching SATISFY.  This understands about
   quoting.  Return the character that caused us to stop searching;
   this is either the same as SATISFY, or 0. */
static int
brace_gobbler (text, tlen, indx, satisfy)
     char *text;
     size_t tlen;
     int *indx;
     int satisfy;
{
  register int i, c, quoted, level, pass_next;
#if defined (SHELL)
  int si;
  char *t;
#endif
  DECLARE_MBSTATE;

  level = quoted = pass_next = 0;

  i = *indx;
  while (c = text[i])
    {
      if (pass_next)
	{
	  pass_next = 0;
	  ADVANCE_CHAR (text, tlen, i);
	  continue;
	}

      /* A backslash escapes the next character.  This allows backslash to
	 escape the quote character in a double-quoted string. */
      if (c == '\\' && (quoted == 0 || quoted == '"' || quoted == '`'))
	{
	  pass_next = 1;
	  i++;
	  continue;
	}

      if (quoted)
	{
	  if (c == quoted)
	    quoted = 0;
	  ADVANCE_CHAR (text, tlen, i);
	  continue;
	}

      if (c == '"' || c == '\'' || c == '`')
	{
	  quoted = c;
	  i++;
	  continue;
	}

#if defined (SHELL)
      /* Pass new-style command substitutions through unchanged. */
      if (c == '$' && text[i+1] == '(')			/* ) */
	{
	  si = i + 2;
	  t = extract_command_subst (text, &si);
	  i = si;
	  free (t);
	  i++;
	  continue;
	}
#endif

      if (c == satisfy && level == 0 && quoted == 0)
	{
	  /* We ignore an open brace surrounded by whitespace, and also
	     an open brace followed immediately by a close brace preceded
	     by whitespace.  */
	  if (c == '{' &&
	      ((!i || brace_whitespace (text[i - 1])) &&
	       (brace_whitespace (text[i + 1]) || text[i + 1] == '}')))
	    {
	      i++;
	      continue;
	    }
#if defined (SHELL)
	  /* If this is being compiled as part of bash, ignore the `{'
	     in a `${}' construct */
	  if ((c != '{') || i == 0 || (text[i - 1] != '$'))
#endif /* SHELL */
	    break;
	}

      if (c == '{')
	level++;
      else if (c == '}' && level)
	level--;

      ADVANCE_CHAR (text, tlen, i);
    }

  *indx = i;
  return (c);
}

/* Return a new array of strings which is the result of appending each
   string in ARR2 to each string in ARR1.  The resultant array is
   len (arr1) * len (arr2) long.  For convenience, ARR1 (and its contents)
   are free ()'ed.  ARR1 can be NULL, in that case, a new version of ARR2
   is returned. */
static char **
array_concat (arr1, arr2)
     char **arr1, **arr2;
{
  register int i, j, len, len1, len2;
  register char **result;

  if (arr1 == 0)
    return (strvec_copy (arr2));

  if (arr2 == 0)
    return (strvec_copy (arr1));

  len1 = strvec_len (arr1);
  len2 = strvec_len (arr2);

  result = (char **)xmalloc ((1 + (len1 * len2)) * sizeof (char *));

  len = 0;
  for (i = 0; i < len1; i++)
    {
      int strlen_1 = strlen (arr1[i]);

      for (j = 0; j < len2; j++)
	{
	  result[len] =
	    (char *)xmalloc (1 + strlen_1 + strlen (arr2[j]));
	  strcpy (result[len], arr1[i]);
	  strcpy (result[len] + strlen_1, arr2[j]);
	  len++;
	}
      free (arr1[i]);
    }
  free (arr1);

  result[len] = (char *)NULL;
  return (result);
}

#if defined (TEST)
#include <stdio.h>

fatal_error (format, arg1, arg2)
     char *format, *arg1, *arg2;
{
  report_error (format, arg1, arg2);
  exit (1);
}

report_error (format, arg1, arg2)
     char *format, *arg1, *arg2;
{
  fprintf (stderr, format, arg1, arg2);
  fprintf (stderr, "\n");
}

main ()
{
  char example[256];

  for (;;)
    {
      char **result;
      int i;

      fprintf (stderr, "brace_expand> ");

      if ((!fgets (example, 256, stdin)) ||
	  (strncmp (example, "quit", 4) == 0))
	break;

      if (strlen (example))
	example[strlen (example) - 1] = '\0';

      result = brace_expand (example);

      for (i = 0; result[i]; i++)
	printf ("%s\n", result[i]);

      free_array (result);
    }
}

/*
 * Local variables:
 * compile-command: "gcc -g -Bstatic -DTEST -o brace_expand braces.c general.o"
 * end:
 */

#endif /* TEST */
#endif /* BRACE_EXPANSION */
