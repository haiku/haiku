/* strmatch.c -- ksh-like extended pattern matching for the shell and filename
		globbing. */

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

#include <config.h>

#include "stdc.h"
#include "strmatch.h"

/* Structured this way so that if HAVE_LIBC_FNM_EXTMATCH is defined, the
   matching portion of the library (smatch.c) is not linked into the shell. */

#ifndef HAVE_LIBC_FNM_EXTMATCH
extern int xstrmatch __P((char *, char *, int));
#else
#  define xstrmatch fnmatch
#endif

int
strmatch (pattern, string, flags)
     char *pattern;
     char *string;
     int flags;
{
  if (string == 0 || pattern == 0)
    return FNM_NOMATCH;

  return (xstrmatch (pattern, string, flags));
}

#ifdef TEST
main (c, v)
     int c;
     char **v;
{
  char *string, *pat;

  string = v[1];
  pat = v[2];

  if (strmatch (pat, string, 0) == 0)
    {
      printf ("%s matches %s\n", string, pat);
      exit (0);
    }
  else
    {
      printf ("%s does not match %s\n", string, pat);
      exit (1);
    }
}
#endif
