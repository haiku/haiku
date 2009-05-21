/* printf wrappers that fail immediately for non-file-related errors
   Copyright (C) 2007-2008 Free Software Foundation, Inc.

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

#ifndef _XPRINTF_H
#define _XPRINTF_H

#include <stdarg.h>
#include <stdio.h>

#ifndef __attribute__
/* This feature is available in gcc versions 2.5 and later.  */
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 5)
#  define __attribute__(Spec)	/* empty */
# endif
/* The __-protected variants of `format' and `printf' attributes
   are accepted by gcc versions 2.6.4 (effectively 2.7) and later.  */
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 7)
#  define __format__ format
#  define __printf__ printf
# endif
#endif

extern int xprintf (char const *restrict format, ...)
  __attribute__ ((__format__ (__printf__, 1, 2)));
extern int xvprintf (char const *restrict format, va_list args)
  __attribute__ ((__format__ (__printf__, 1, 0)));
extern int xfprintf (FILE *restrict stream, char const *restrict format, ...)
  __attribute__ ((__format__ (__printf__, 2, 3)));
extern int xvfprintf (FILE *restrict stream, char const *restrict format,
		      va_list args)
  __attribute__ ((__format__ (__printf__, 2, 0)));

#endif
