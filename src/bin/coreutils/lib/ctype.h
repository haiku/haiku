/* DO NOT EDIT! GENERATED AUTOMATICALLY! */
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
#pragma GCC system_header
#endif

/* Include the original <ctype.h>.  */
/* The include_next requires a split double-inclusion guard.  */
#include_next <ctype.h>

#ifndef _GL_CTYPE_H
#define _GL_CTYPE_H

/* The definition of GL_LINK_WARNING is copied here.  */
/* GL_LINK_WARNING("literal string") arranges to emit the literal string as
   a linker warning on most glibc systems.
   We use a linker warning rather than a preprocessor warning, because
   #warning cannot be used inside macros.  */
#ifndef GL_LINK_WARNING
  /* This works on platforms with GNU ld and ELF object format.
     Testing __GLIBC__ is sufficient for asserting that GNU ld is in use.
     Testing __ELF__ guarantees the ELF object format.
     Testing __GNUC__ is necessary for the compound expression syntax.  */
# if defined __GLIBC__ && defined __ELF__ && defined __GNUC__
#  define GL_LINK_WARNING(message) \
     GL_LINK_WARNING1 (__FILE__, __LINE__, message)
#  define GL_LINK_WARNING1(file, line, message) \
     GL_LINK_WARNING2 (file, line, message)  /* macroexpand file and line */
#  define GL_LINK_WARNING2(file, line, message) \
     GL_LINK_WARNING3 (file ":" #line ": warning: " message)
#  define GL_LINK_WARNING3(message) \
     ({ static const char warning[sizeof (message)]             \
          __attribute__ ((__unused__,                           \
                          __section__ (".gnu.warning"),         \
                          __aligned__ (1)))                     \
          = message "\n";                                       \
        (void)0;                                                \
     })
# else
#  define GL_LINK_WARNING(message) ((void) 0)
# endif
#endif

/* Return non-zero if c is a blank, i.e. a space or tab character.  */
#if 1
# if !1
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
