/*
 * argz.h
 *    The functions below are "borrowed" from glibc-2.2.3 (argz-next.c).
 *    This has been done to make atftp compile with uclibc, BSD, Solaris
 *    and other platform without glic
 *
 *
 * $Id: argz.c,v 1.1 2003/01/21 01:38:35 jp Exp $
 *
 * Copyright (c) 2000 Jean-Pierre Lefebvre <helix@step.polymtl.ca>
 *                and Remi Lefebvre <remi@debian.org>
 *
 * atftp is free software; you can redistribute them and/or modify them
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 */

#ifndef HAVE_ARGZ

/* Routines for dealing with '\0' separated arg vectors.
   Copyright (C) 1995, 96, 97, 98, 99, 2000 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <stdlib.h>
#include "argz.h"

char * argz_next (const char *argz, size_t argz_len, const char *entry)
{
  if (entry)
    {
      if (entry < argz + argz_len)
        entry = strchr (entry, '\0') + 1;

      return entry >= argz + argz_len ? NULL : (char *) entry;
    }
  else
    if (argz_len > 0)
      return (char *) argz;
    else
      return NULL;
}

error_t argz_create_sep (const char *string, int delim, char **argz, size_t *len)
{
  size_t nlen = strlen (string) + 1;

  if (nlen > 1)
    {
      const char *rp;
      char *wp;

      *argz = (char *) malloc (nlen);
      if (*argz == NULL)
	return ENOMEM;

      rp = string;
      wp = *argz;
      do
	if (*rp == delim)
	  {
	    if (wp > *argz && wp[-1] != '\0')
	      *wp++ = '\0';
	    else
	      --nlen;
	  }
	else
	  *wp++ = *rp;
      while (*rp++ != '\0');

      if (nlen == 0)
	{
	  free (*argz);
	  *argz = NULL;
	  *len = 0;
	}

      *len = nlen;
    }
  else
    {
      *argz = NULL;
      *len = 0;
    }

  return 0;
}

size_t argz_count (const char *argz, size_t len)
{
  size_t count = 0;
  while (len > 0)
    {
      size_t part_len = strlen(argz);
      argz += part_len + 1;
      len -= part_len + 1;
      count++;
    }
  return count;
}

/* Puts pointers to each string in ARGZ, plus a terminating 0 element, into
   ARGV, which must be large enough to hold them all.  */
void argz_extract (const char *argz, size_t len, char **argv)
{
  while (len > 0)
    {
      size_t part_len = strlen (argz);
      *argv++ = (char *) argz;
      argz += part_len + 1;
      len -= part_len + 1;
    }
  *argv = 0;
}

#endif
