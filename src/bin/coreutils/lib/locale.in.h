/* A POSIX <locale.h>.
   Copyright (C) 2007-2010 Free Software Foundation, Inc.

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

#ifndef _GL_LOCALE_H

#if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
#endif

/* The include_next requires a split double-inclusion guard.  */
#@INCLUDE_NEXT@ @NEXT_LOCALE_H@

#ifndef _GL_LOCALE_H
#define _GL_LOCALE_H

/* NetBSD 5.0 mis-defines NULL.  */
#include <stddef.h>

/* MacOS X 10.5 defines the locale_t type in <xlocale.h>.  */
#if @HAVE_XLOCALE_H@
# include <xlocale.h>
#endif

/* The definition of GL_LINK_WARNING is copied here.  */

/* The definition of _GL_ARG_NONNULL is copied here.  */

/* The LC_MESSAGES locale category is specified in POSIX, but not in ISO C.
   On systems that don't define it, use the same value as GNU libintl.  */
#if !defined LC_MESSAGES
# define LC_MESSAGES 1729
#endif

#if @GNULIB_DUPLOCALE@
# if @REPLACE_DUPLOCALE@
#  undef duplocale
#  define duplocale rpl_duplocale
extern locale_t duplocale (locale_t locale) _GL_ARG_NONNULL ((1));
# endif
#elif defined GNULIB_POSIXCHECK
# undef duplocale
# define duplocale(l) \
   (GL_LINK_WARNING ("duplocale is buggy on some glibc systems - " \
                     "use gnulib module duplocale for portability"), \
    duplocale (l))
#endif

#endif /* _GL_LOCALE_H */
#endif /* _GL_LOCALE_H */
