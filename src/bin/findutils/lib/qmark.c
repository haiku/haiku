/* pred.c -- execute the expression tree.
   Copyright (C) 1990, 1991, 1992, 1993, 1994, 2000, 
                 2003, 2004 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

# include <stddef.h>
# include <stdlib.h>
#include <ctype.h>

#if HAVE_STRING_H || STDC_HEADERS
#include <string.h>
#else
#include <strings.h>
#endif


/* Get mbstate_t, mbrtowc(), mbsinit(), wcwidth().  */
#if HAVE_WCHAR_H
# include <wchar.h>
#endif

#include "printquoted.h"


/* 
   This comment, IN_CTYPE_DOMAIN and ISPRINT were borrowed from 
   coreutils at Sun Jun  5 21:17:40 2005 UTC.

   Jim Meyering writes:

   "... Some ctype macros are valid only for character codes that
   isascii says are ASCII (SGI's IRIX-4.0.5 is one such system --when
   using /bin/cc or gcc but without giving an ansi option).  So, all
   ctype uses should be through macros like ISPRINT...  If
   STDC_HEADERS is defined, then autoconf has verified that the ctype
   macros don't need to be guarded with references to isascii. ...
   Defining isascii to 1 should let any compiler worth its salt
   eliminate the && through constant folding."

   Bruno Haible adds:

   "... Furthermore, isupper(c) etc. have an undefined result if c is
   outside the range -1 <= c <= 255. One is tempted to write isupper(c)
   with c being of type `char', but this is wrong if c is an 8-bit
   character >= 128 which gets sign-extended to a negative value.
   The macro ISUPPER protects against this as well."  */




/* ISPRINT is defined in <sys/euc.h> on at least Solaris2.6 systems.  */
#undef ISPRINT
#define ISPRINT(c) (IN_CTYPE_DOMAIN (c) && isprint (c))

#if STDC_HEADERS || (!defined (isascii) && !HAVE_ISASCII)
# define IN_CTYPE_DOMAIN(c) 1
#else
# define IN_CTYPE_DOMAIN(c) isascii(c)
#endif





/* Convert a possibly-signed character to an unsigned character.  This is
 * a bit safer than casting to unsigned char, since it catches some type
 * errors that the cast doesn't.  
 *
 * This code taken from coreutils' system.h header at 
 * Sun Jun  5 21:05:21 2005 UTC.
 */
static inline unsigned char to_uchar (char ch)
{
  return ch;
}



static size_t
unibyte_qmark_chars(char *buf, size_t len)
{
  char *p = buf;
  char const *plimit = buf + len;
  
  while (p < plimit)
    {
      if (! ISPRINT (to_uchar (*p)))
	*p = '?';
      p++;
    }
  return len;
}


#if HAVE_MBRTOWC
static size_t
multibyte_qmark_chars(char *buf, size_t len)
{
  if (MB_CUR_MAX <= 1)
    {
      return unibyte_qmark_chars(buf, len);
    }
  else
    {
      char const *p = buf;
      char const *plimit = buf + len;
      char *q = buf;

      while (p < plimit)
	switch (*p)
	  {
	  case ' ': case '!': case '"': case '#': case '%':
	  case '&': case '\'': case '(': case ')': case '*':
	  case '+': case ',': case '-': case '.': case '/':
	  case '0': case '1': case '2': case '3': case '4':
	  case '5': case '6': case '7': case '8': case '9':
	  case ':': case ';': case '<': case '=': case '>':
	  case '?':
	  case 'A': case 'B': case 'C': case 'D': case 'E':
	  case 'F': case 'G': case 'H': case 'I': case 'J':
	  case 'K': case 'L': case 'M': case 'N': case 'O':
	  case 'P': case 'Q': case 'R': case 'S': case 'T':
	  case 'U': case 'V': case 'W': case 'X': case 'Y':
	  case 'Z':
	  case '[': case '\\': case ']': case '^': case '_':
	  case 'a': case 'b': case 'c': case 'd': case 'e':
	  case 'f': case 'g': case 'h': case 'i': case 'j':
	  case 'k': case 'l': case 'm': case 'n': case 'o':
	  case 'p': case 'q': case 'r': case 's': case 't':
	  case 'u': case 'v': case 'w': case 'x': case 'y':
	  case 'z': case '{': case '|': case '}': case '~':
	    /* These characters are printable ASCII characters.  */
	    *q++ = *p++;
	    break;
	  default:
	    /* If we have a multibyte sequence, copy it until we
	       reach its end, replacing each non-printable multibyte
	       character with a single question mark.  */
	    {
	      mbstate_t mbstate;
	      memset (&mbstate, 0, sizeof mbstate);
	      do
		{
		  wchar_t wc;
		  size_t bytes;
		  int w;

		  bytes = mbrtowc (&wc, p, plimit - p, &mbstate);

		  if (bytes == (size_t) -1)
		    {
		      /* An invalid multibyte sequence was
			 encountered.  Skip one input byte, and
			 put a question mark.  */
		      p++;
		      *q++ = '?';
		      break;
		    }

		  if (bytes == (size_t) -2)
		    {
		      /* An incomplete multibyte character
			 at the end.  Replace it entirely with
			 a question mark.  */
		      p = plimit;
		      *q++ = '?';
		      break;
		    }

		  if (bytes == 0)
		    /* A null wide character was encountered.  */
		    bytes = 1;

		  w = wcwidth (wc);
		  if (w >= 0)
		    {
		      /* A printable multibyte character.
			 Keep it.  */
		      for (; bytes > 0; --bytes)
			*q++ = *p++;
		    }
		  else
		    {
		      /* An unprintable multibyte character.
			 Replace it entirely with a question
			 mark.  */
		      p += bytes;
		      *q++ = '?';
		    }
		}
	      while (! mbsinit (&mbstate));
	    }
	    break;
	  }

      /* The buffer may have shrunk.  */
      len = q - buf;
      return len;
    }
}
#endif


/* Scan BUF, replacing any dangerous-looking characters with question
 * marks.  This code is taken from the ls.c file in coreutils as at 
 * Sun Jun  5 20:51:54 2005 UTC.
 *
 * This function may shrink the buffer.   Either way, the new length
 * is returned.
 */
size_t
qmark_chars(char *buf, size_t len)
{
#if HAVE_MBRTOWC
  return multibyte_qmark_chars(buf, len);
#else
  return unibyte_qmark_chars(buf, len);
#endif  
}

