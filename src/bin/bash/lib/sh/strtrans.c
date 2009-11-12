/* strtrans.c - Translate and untranslate strings with ANSI-C escape sequences. */

/* Copyright (C) 2000 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Bash is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <config.h>

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include <bashansi.h>
#include <stdio.h>
#include <chartypes.h>

#include "shell.h"

#ifdef ESC
#undef ESC
#endif
#define ESC '\033'	/* ASCII */

/* Convert STRING by expanding the escape sequences specified by the
   ANSI C standard.  If SAWC is non-null, recognize `\c' and use that
   as a string terminator.  If we see \c, set *SAWC to 1 before
   returning.  LEN is the length of STRING.  If (FLAGS&1) is non-zero,
   that we're translating a string for `echo -e', and therefore should not
   treat a single quote as a character that may be escaped with a backslash.
   If (FLAGS&2) is non-zero, we're expanding for the parser and want to
   quote CTLESC and CTLNUL with CTLESC.  If (flags&4) is non-zero, we want
   to remove the backslash before any unrecognized escape sequence. */
char *
ansicstr (string, len, flags, sawc, rlen)
     char *string;
     int len, flags, *sawc, *rlen;
{
  int c, temp;
  char *ret, *r, *s;

  if (string == 0 || *string == '\0')
    return ((char *)NULL);

  ret = (char *)xmalloc (2*len + 1);	/* 2*len for possible CTLESC */
  for (r = ret, s = string; s && *s; )
    {
      c = *s++;
      if (c != '\\' || *s == '\0')
	*r++ = c;
      else
	{
	  switch (c = *s++)
	    {
#if defined (__STDC__)
	    case 'a': c = '\a'; break;
	    case 'v': c = '\v'; break;
#else
	    case 'a': c = '\007'; break;
	    case 'v': c = (int) 0x0B; break;
#endif
	    case 'b': c = '\b'; break;
	    case 'e': case 'E':		/* ESC -- non-ANSI */
	      c = ESC; break;
	    case 'f': c = '\f'; break;
	    case 'n': c = '\n'; break;
	    case 'r': c = '\r'; break;
	    case 't': c = '\t'; break;
	    case '1': case '2': case '3':
	    case '4': case '5': case '6':
	    case '7':
#if 1
	      if (flags & 1)
		{
		  *r++ = '\\';
		  break;
		}
	    /*FALLTHROUGH*/
#endif
	    case '0':
	      /* If (FLAGS & 1), we're translating a string for echo -e (or
		 the equivalent xpg_echo option), so we obey the SUSv3/
		 POSIX-2001 requirement and accept 0-3 octal digits after
		 a leading `0'. */
	      temp = 2 + ((flags & 1) && (c == '0'));
	      for (c -= '0'; ISOCTAL (*s) && temp--; s++)
		c = (c * 8) + OCTVALUE (*s);
	      c &= 0xFF;
	      break;
	    case 'x':			/* Hex digit -- non-ANSI */
	      if ((flags & 2) && *s == '{')
		{
		  flags |= 16;		/* internal flag value */
		  s++;
		}
	      /* Consume at least two hex characters */
	      for (temp = 2, c = 0; ISXDIGIT ((unsigned char)*s) && temp--; s++)
		c = (c * 16) + HEXVALUE (*s);
	      /* DGK says that after a `\x{' ksh93 consumes ISXDIGIT chars
		 until a non-xdigit or `}', so potentially more than two
		 chars are consumed. */
	      if (flags & 16)
		{
		  for ( ; ISXDIGIT ((unsigned char)*s); s++)
		    c = (c * 16) + HEXVALUE (*s);
		  flags &= ~16;
		  if (*s == '}')
		    s++;
	        }
	      /* \x followed by non-hex digits is passed through unchanged */
	      else if (temp == 2)
		{
		  *r++ = '\\';
		  c = 'x';
		}
	      c &= 0xFF;
	      break;
	    case '\\':
	      break;
	    case '\'': case '"': case '?':
	      if (flags & 1)
		*r++ = '\\';
	      break;
	    case 'c':
	      if (sawc)
		{
		  *sawc = 1;
		  *r = '\0';
		  if (rlen)
		    *rlen = r - ret;
		  return ret;
		}
	      else if ((flags & 1) == 0 && (c = *s))
		{
		  s++;
		  c = TOCTRL(c);
		  break;
		}
		/*FALLTHROUGH*/
	    default:
		if ((flags & 4) == 0)
		  *r++ = '\\';
		break;
	    }
	  if ((flags & 2) && (c == CTLESC || c == CTLNUL))
	    *r++ = CTLESC;
	  *r++ = c;
	}
    }
  *r = '\0';
  if (rlen)
    *rlen = r - ret;
  return ret;
}

/* Take a string STR, possibly containing non-printing characters, and turn it
   into a $'...' ANSI-C style quoted string.  Returns a new string. */
char *
ansic_quote (str, flags, rlen)
     char *str;
     int flags, *rlen;
{
  char *r, *ret, *s;
  int l, rsize;
  unsigned char c;

  if (str == 0 || *str == 0)
    return ((char *)0);

  l = strlen (str);
  rsize = 4 * l + 4;
  r = ret = (char *)xmalloc (rsize);

  *r++ = '$';
  *r++ = '\'';

  for (s = str, l = 0; *s; s++)
    {
      c = *s;
      l = 1;		/* 1 == add backslash; 0 == no backslash */
      switch (c)
	{
	case ESC: c = 'E'; break;
#ifdef __STDC__
	case '\a': c = 'a'; break;
	case '\v': c = 'v'; break;
#else
	case '\007': c = 'a'; break;
	case 0x0b: c = 'v'; break;
#endif

	case '\b': c = 'b'; break;
	case '\f': c = 'f'; break;
	case '\n': c = 'n'; break;
	case '\r': c = 'r'; break;
	case '\t': c = 't'; break;
	case '\\':
	case '\'':
	  break;
	default:
	  if (ISPRINT (c) == 0)
	    {
	      *r++ = '\\';
	      *r++ = TOCHAR ((c >> 6) & 07);
	      *r++ = TOCHAR ((c >> 3) & 07);
	      *r++ = TOCHAR (c & 07);
	      continue;
	    }
	  l = 0;
	  break;
	}
      if (l)
	*r++ = '\\';
      *r++ = c;
    }

  *r++ = '\'';
  *r = '\0';
  if (rlen)
    *rlen = r - ret;
  return ret;
}

/* return 1 if we need to quote with $'...' because of non-printing chars. */
int
ansic_shouldquote (string)
     const char *string;
{
  const char *s;
  unsigned char c;

  if (string == 0)
    return 0;

  for (s = string; c = *s; s++)
    if (ISPRINT (c) == 0)
      return 1;

  return 0;
}

/* $'...' ANSI-C expand the portion of STRING between START and END and
   return the result.  The result cannot be longer than the input string. */
char *
ansiexpand (string, start, end, lenp)
     char *string;
     int start, end, *lenp;
{
  char *temp, *t;
  int len, tlen;

  temp = (char *)xmalloc (end - start + 1);
  for (tlen = 0, len = start; len < end; )
    temp[tlen++] = string[len++];
  temp[tlen] = '\0';

  if (*temp)
    {
      t = ansicstr (temp, tlen, 2, (int *)NULL, lenp);
      free (temp);
      return (t);
    }
  else
    {
      if (lenp)
	*lenp = 0;
      return (temp);
    }
}
