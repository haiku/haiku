/* Copyright (C) 1999 Free Software Foundation, Inc.

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

#include <config.h>

#if defined (HAVE_UNISTD_H)
#  ifdef _MINIX
#    include <sys/types.h>
#  endif
#  include <unistd.h>
#endif

#include <stdio.h>

#include "syntax.h"
#include <xmalloc.h>

/* **************************************************************** */
/*								    */
/*	 Functions for quoting strings to be re-read as input	    */
/*								    */
/* **************************************************************** */

/* Return a new string which is the single-quoted version of STRING.
   Used by alias and trap, among others. */
char *
sh_single_quote (string)
     char *string;
{
  register int c;
  char *result, *r, *s;

  result = (char *)xmalloc (3 + (4 * strlen (string)));
  r = result;
  *r++ = '\'';

  for (s = string; s && (c = *s); s++)
    {
      *r++ = c;

      if (c == '\'')
	{
	  *r++ = '\\';	/* insert escaped single quote */
	  *r++ = '\'';
	  *r++ = '\'';	/* start new quoted string */
	}
    }

  *r++ = '\'';
  *r = '\0';

  return (result);
}

/* Quote STRING using double quotes.  Return a new string. */
char *
sh_double_quote (string)
     char *string;
{
  register unsigned char c;
  char *result, *r, *s;

  result = (char *)xmalloc (3 + (2 * strlen (string)));
  r = result;
  *r++ = '"';

  for (s = string; s && (c = *s); s++)
    {
      if (sh_syntaxtab[c] & CBSDQUOTE)
	*r++ = '\\';

      *r++ = c;
    }

  *r++ = '"';
  *r = '\0';

  return (result);
}

/* Remove backslashes that are quoting characters that are special between
   double quotes.  Return a new string. */
char *
sh_un_double_quote (string)
     char *string;
{
  register int c, pass_next;
  char *result, *r, *s;

  r = result = (char *)xmalloc (strlen (string) + 1);

  for (pass_next = 0, s = string; s && (c = *s); s++)
    {
      if (pass_next)
	{
	  *r++ = c;
	  pass_next = 0;
	  continue;
	}
      if (c == '\\' && (sh_syntaxtab[(unsigned char) s[1]] & CBSDQUOTE))
	{
	  pass_next = 1;
	  continue;
	}
      *r++ = c;
    }

  *r = '\0';
  return result;
}

/* Quote special characters in STRING using backslashes.  Return a new
   string. */
char *
sh_backslash_quote (string)
     char *string;
{
  int c;
  char *result, *r, *s;

  result = (char *)xmalloc (2 * strlen (string) + 1);

  for (r = result, s = string; s && (c = *s); s++)
    {
      switch (c)
	{
	case ' ': case '\t': case '\n':		/* IFS white space */
	case '\'': case '"': case '\\':		/* quoting chars */
	case '|': case '&': case ';':		/* shell metacharacters */
	case '(': case ')': case '<': case '>':
	case '!': case '{': case '}':		/* reserved words */
	case '*': case '[': case '?': case ']':	/* globbing chars */
	case '^':
	case '$': case '`':			/* expansion chars */
	case ',':				/* brace expansion */
	  *r++ = '\\';
	  *r++ = c;
	  break;
#if 0
	case '~':				/* tilde expansion */
	  if (s == string || s[-1] == '=' || s[-1] == ':')
	    *r++ = '\\';
	  *r++ = c;
	  break;
#endif
	case '#':				/* comment char */
	  if (s == string)
	    *r++ = '\\';
	  /* FALLTHROUGH */
	default:
	  *r++ = c;
	  break;
	}
    }

  *r = '\0';
  return (result);
}

#if defined (PROMPT_STRING_DECODE)
/* Quote characters that get special treatment when in double quotes in STRING
   using backslashes.  Return a new string. */
char *
sh_backslash_quote_for_double_quotes (string)
     char *string;
{
  unsigned char c;
  char *result, *r, *s;

  result = (char *)xmalloc (2 * strlen (string) + 1);

  for (r = result, s = string; s && (c = *s); s++)
    {
      if (sh_syntaxtab[c] & CBSDQUOTE)
	*r++ = '\\';

      *r++ = c;
    }

  *r = '\0';
  return (result);
}
#endif /* PROMPT_STRING_DECODE */

int
sh_contains_shell_metas (string)
     char *string;
{
  char *s;

  for (s = string; s && *s; s++)
    {
      switch (*s)
	{
	case ' ': case '\t': case '\n':		/* IFS white space */
	case '\'': case '"': case '\\':		/* quoting chars */
	case '|': case '&': case ';':		/* shell metacharacters */
	case '(': case ')': case '<': case '>':
	case '!': case '{': case '}':		/* reserved words */
	case '*': case '[': case '?': case ']':	/* globbing chars */
	case '^':
	case '$': case '`':			/* expansion chars */
	  return (1);
	case '~':				/* tilde expansion */
	  if (s == string || s[-1] == '=' || s[-1] == ':')
	    return (1);
	  break;
	case '#':
	  if (s == string)			/* comment char */
	    return (1);
	  /* FALLTHROUGH */
	default:
	  break;
	}
    }

  return (0);
}
