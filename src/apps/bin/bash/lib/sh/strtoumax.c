/* Convert string representation of a number into an uintmax_t value.
   Copyright 1999, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Written by Paul Eggert.  Modified by Chet Ramey for Bash. */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#if HAVE_INTTYPES_H
#  include <inttypes.h>
#endif

#if HAVE_STDLIB_H
#  include <stdlib.h>
#endif

#include <stdc.h>

/* Verify a requirement at compile-time (unlike assert, which is runtime).  */
#define verify(name, assertion) struct name { char a[(assertion) ? 1 : -1]; }

#ifndef HAVE_DECL_STRTOUL
"this configure-time declaration test was not run"
#endif
#if !HAVE_DECL_STRTOUL
extern unsigned long strtoul __P((const char *, char **, int));
#endif

#ifndef HAVE_DECL_STRTOULL
"this configure-time declaration test was not run"
#endif
#if !HAVE_DECL_STRTOULL && HAVE_UNSIGNED_LONG_LONG
extern unsigned long long strtoull __P((const char *, char **, int));
#endif

uintmax_t
strtoumax (ptr, endptr, base)
     const char *ptr;
     char **endptr;
     int base;
{
#if HAVE_UNSIGNED_LONG_LONG
  verify (size_is_that_of_unsigned_long_or_unsigned_long_long,
	  (sizeof (uintmax_t) == sizeof (unsigned long) ||
	   sizeof (uintmax_t) == sizeof (unsigned long long)));

  if (sizeof (uintmax_t) != sizeof (unsigned long))
    return (strtoull (ptr, endptr, base));
#else
  verify (size_is_that_of_unsigned_long, sizeof (uintmax_t) == sizeof (unsigned long));
#endif

  return (strtoul (ptr, endptr, base));
}

#ifdef TESTING
# include <stdio.h>
int
main ()
{
  char *p, *endptr;
  uintmax_t x;
#if HAVE_UNSIGNED_LONG_LONG
  unsigned long long y;
#endif
  unsigned long z;

  printf ("sizeof uintmax_t: %d\n", sizeof (uintmax_t));

#if HAVE_UNSIGNED_LONG_LONG
  printf ("sizeof unsigned long long: %d\n", sizeof (unsigned long long));
#endif
  printf ("sizeof unsigned long: %d\n", sizeof (unsigned long));

  x = strtoumax("42", &endptr, 10);
#if HAVE_LONG_LONG
  y = strtoull("42", &endptr, 10);
#else
  y = 0;
#endif
  z = strtoul("42", &endptr, 10);

  printf ("%llu %llu %lu\n", x, y, z);

  exit (0);
}
#endif
