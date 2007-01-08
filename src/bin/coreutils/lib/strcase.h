/* Case-insensitive string comparison functions.
   Copyright (C) 1995-1996, 2001, 2003, 2005-2006 Free Software Foundation, Inc.

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
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef _STRCASE_H
#define _STRCASE_H

#include <stddef.h>
/* Include header files with a possibly conflicting declarations of strcasecmp
   before we define it as a macro, so that they will be no-ops if included
   after strcasecmp is defined as a macro.  */
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif


/* No known system has a strcasecmp() function that works correctly in
   multibyte locales.  Therefore we use our version always.  */
#define strcasecmp rpl_strcasecmp
/* Compare strings S1 and S2, ignoring case, returning less than, equal to or
   greater than zero if S1 is lexicographically less than, equal to or greater
   than S2.
   Note: This function may, in multibyte locales, return 0 for strings of
   different lengths!  */
extern int strcasecmp (const char *s1, const char *s2);

/* Compare no more than N characters of strings S1 and S2, ignoring case,
   returning less than, equal to or greater than zero if S1 is
   lexicographically less than, equal to or greater than S2.
   Note: This function can not work correctly in multibyte locales.  */
#if ! HAVE_DECL_STRNCASECMP
extern int strncasecmp (const char *s1, const char *s2, size_t n);
#endif


#ifdef __cplusplus
}
#endif


#endif /* _STRCASE_H */
