/* A more useful interface to strtol.

   Copyright (C) 1995, 1996, 1998, 1999, 2001, 2002, 2003, 2004, 2006, 2007,
   2008 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef XSTRTOL_H_
# define XSTRTOL_H_ 1

# include <getopt.h>
# include <inttypes.h>

# ifndef _STRTOL_ERROR
enum strtol_error
  {
    LONGINT_OK = 0,

    /* These two values can be ORed together, to indicate that both
       errors occurred.  */
    LONGINT_OVERFLOW = 1,
    LONGINT_INVALID_SUFFIX_CHAR = 2,

    LONGINT_INVALID_SUFFIX_CHAR_WITH_OVERFLOW = (LONGINT_INVALID_SUFFIX_CHAR
						 | LONGINT_OVERFLOW),
    LONGINT_INVALID = 4
  };
typedef enum strtol_error strtol_error;
# endif

# define _DECLARE_XSTRTOL(name, type) \
  strtol_error name (const char *, char **, int, type *, const char *);
_DECLARE_XSTRTOL (xstrtol, long int)
_DECLARE_XSTRTOL (xstrtoul, unsigned long int)
_DECLARE_XSTRTOL (xstrtoimax, intmax_t)
_DECLARE_XSTRTOL (xstrtoumax, uintmax_t)

#ifndef __attribute__
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 8)
#  define __attribute__(x)
# endif
#endif

#ifndef ATTRIBUTE_NORETURN
# define ATTRIBUTE_NORETURN __attribute__ ((__noreturn__))
#endif

/* Report an error for an invalid integer in an option argument.

   ERR is the error code returned by one of the xstrto* functions.

   Use OPT_IDX to decide whether to print the short option string "C"
   or "-C" or a long option string derived from LONG_OPTION.  OPT_IDX
   is -2 if the short option "C" was used, without any leading "-"; it
   is -1 if the short option "-C" was used; otherwise it is an index
   into LONG_OPTIONS, which should have a name preceded by two '-'
   characters.

   ARG is the option-argument containing the integer.

   After reporting an error, exit with a failure status.  */

void xstrtol_fatal (enum strtol_error,
		    int, char, struct option const *,
		    char const *) ATTRIBUTE_NORETURN;

#endif /* not XSTRTOL_H_ */
