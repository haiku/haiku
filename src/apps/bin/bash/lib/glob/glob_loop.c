/* Copyright (C) 1991-2002 Free Software Foundation, Inc.

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

static int INTERNAL_GLOB_PATTERN_P __P((const CHAR *));

/* Return nonzero if PATTERN has any special globbing chars in it.
   Compiled twice, once each for single-byte and multibyte characters. */
static int
INTERNAL_GLOB_PATTERN_P (pattern)
     const CHAR *pattern;
{
  register const CHAR *p;
  register CHAR c;
  int bopen;

  p = pattern;
  bopen = 0;

  while ((c = *p++) != L('\0'))
    switch (c)
      {
      case L('?'):
      case L('*'):
	return 1;

      case L('['):      /* Only accept an open brace if there is a close */
	bopen++;        /* brace to match it.  Bracket expressions must be */
	continue;       /* complete, according to Posix.2 */
      case L(']'):
	if (bopen)
	  return 1;
	continue;

      case L('+'):         /* extended matching operators */
      case L('@'):
      case L('!'):
	if (*p == L('('))  /*) */
	  return 1;
	continue;

      case L('\\'):
	if (*p++ == L('\0'))
	  return 0;
      }

  return 0;
}

#undef INTERNAL_GLOB_PATTERN_P
#undef L
#undef INT
#undef CHAR
