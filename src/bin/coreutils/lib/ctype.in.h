/* -*- buffer-read-only: t -*- vi: set ro: */
/* DO NOT EDIT! GENERATED AUTOMATICALLY! */
#line 1
/* A substitute for ISO C99 <ctype.h>, for platforms on which it is incomplete.

   Copyright (C) 2009, 2010 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Written by Bruno Haible.  */

/*
 * ISO C 99 <ctype.h> for platforms on which it is incomplete.
 * <http://www.opengroup.org/onlinepubs/9699919799/basedefs/ctype.h.html>
 */

#ifndef _GL_CTYPE_H

#if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
#endif

/* Include the original <ctype.h>.  */
/* The include_next requires a split double-inclusion guard.  */
#@INCLUDE_NEXT@ @NEXT_CTYPE_H@

#ifndef _GL_CTYPE_H
#define _GL_CTYPE_H

/* The definition of GL_LINK_WARNING is copied here.  */

/* Return non-zero if c is a blank, i.e. a space or tab character.  */
#if @GNULIB_ISBLANK@
# if !@HAVE_ISBLANK@
extern int isblank (int c);
# endif
#elif defined GNULIB_POSIXCHECK
# undef isblank
# define isblank(c) \
    (GL_LINK_WARNING ("isblank is unportable - " \
                      "use gnulib module isblank for portability"), \
     isblank (c))
#endif

#endif /* _GL_CTYPE_H */
#endif /* _GL_CTYPE_H */
