/* inttostr.h -- convert integers to printable strings

   Copyright (C) 2001 Free Software Foundation, Inc.

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

/* Written by Paul Eggert */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if HAVE_INTTYPES_H
# include <inttypes.h>
#endif

#if HAVE_LIMITS_H
# include <limits.h>
#endif
#ifndef CHAR_BIT
# define CHAR_BIT 8
#endif

#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#define TYPE_SIGNED(t) (! ((t) 0 < (t) -1))

/* Upper bound on the string length of an integer converted to string.
   302 / 1000 is ceil (log10 (2.0)).  Subtract 1 for the sign bit;
   add 1 for integer division truncation; add 1 more for a minus sign.  */
#define INT_STRLEN_BOUND(t) \
 ((sizeof (t) * CHAR_BIT - TYPE_SIGNED (t)) * 302 / 1000 + 1 + TYPE_SIGNED (t))

#define INT_BUFSIZE_BOUND(t) (INT_STRLEN_BOUND (t) + 1)

#ifndef PARAMS
# if defined PROTOTYPES || defined __STDC__
#  define PARAMS(Args) Args
# else
#  define PARAMS(Args) ()
# endif
#endif

char *offtostr PARAMS ((off_t, char *));
char *imaxtostr PARAMS ((intmax_t, char *));
char *umaxtostr PARAMS ((uintmax_t, char *));
