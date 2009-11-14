/* -*- buffer-read-only: t -*- vi: set ro: */
/* DO NOT EDIT! GENERATED AUTOMATICALLY! */
#line 1
/* A substitute for ISO C99 <wctype.h>, for platforms that lack it.

   Copyright (C) 2006-2009 Free Software Foundation, Inc.

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

/* Written by Bruno Haible and Paul Eggert.  */

/*
 * ISO C 99 <wctype.h> for platforms that lack it.
 * <http://www.opengroup.org/susv3xbd/wctype.h.html>
 *
 * iswctype, towctrans, towlower, towupper, wctrans, wctype,
 * wctrans_t, and wctype_t are not yet implemented.
 */

#ifndef _GL_WCTYPE_H

#if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
#endif

#if @HAVE_WINT_T@
/* Solaris 2.5 has a bug: <wchar.h> must be included before <wctype.h>.
   Tru64 with Desktop Toolkit C has a bug: <stdio.h> must be included before
   <wchar.h>.
   BSD/OS 4.0.1 has a bug: <stddef.h>, <stdio.h> and <time.h> must be
   included before <wchar.h>.  */
# include <stddef.h>
# include <stdio.h>
# include <time.h>
# include <wchar.h>
#endif

/* Include the original <wctype.h> if it exists.
   BeOS 5 has the functions but no <wctype.h>.  */
/* The include_next requires a split double-inclusion guard.  */
#if @HAVE_WCTYPE_H@
# @INCLUDE_NEXT@ @NEXT_WCTYPE_H@
#endif

#ifndef _GL_WCTYPE_H
#define _GL_WCTYPE_H

/* Define wint_t.  (Also done in wchar.in.h.)  */
#if !@HAVE_WINT_T@ && !defined wint_t
# define wint_t int
# ifndef WEOF
#  define WEOF -1
# endif
#endif

/* FreeBSD 4.4 to 4.11 has <wctype.h> but lacks the functions.
   Linux libc5 has <wctype.h> and the functions but they are broken.
   Assume all 12 functions are implemented the same way, or not at all.  */
#if ! @HAVE_ISWCNTRL@ || @REPLACE_ISWCNTRL@

/* IRIX 5.3 has macros but no functions, its isw* macros refer to an
   undefined variable _ctmp_ and to <ctype.h> macros like _P, and they
   refer to system functions like _iswctype that are not in the
   standard C library.  Rather than try to get ancient buggy
   implementations like this to work, just disable them.  */
#  undef iswalnum
#  undef iswalpha
#  undef iswblank
#  undef iswcntrl
#  undef iswdigit
#  undef iswgraph
#  undef iswlower
#  undef iswprint
#  undef iswpunct
#  undef iswspace
#  undef iswupper
#  undef iswxdigit
#  undef towlower
#  undef towupper

/* Linux libc5 has <wctype.h> and the functions but they are broken.  */
#  if @REPLACE_ISWCNTRL@
#   define iswalnum rpl_iswalnum
#   define iswalpha rpl_iswalpha
#   define iswblank rpl_iswblank
#   define iswcntrl rpl_iswcntrl
#   define iswdigit rpl_iswdigit
#   define iswgraph rpl_iswgraph
#   define iswlower rpl_iswlower
#   define iswprint rpl_iswprint
#   define iswpunct rpl_iswpunct
#   define iswspace rpl_iswspace
#   define iswupper rpl_iswupper
#   define iswxdigit rpl_iswxdigit
#   define towlower rpl_towlower
#   define towupper rpl_towupper
#  endif

static inline int
iswalnum (wint_t wc)
{
  return ((wc >= '0' && wc <= '9')
	  || ((wc & ~0x20) >= 'A' && (wc & ~0x20) <= 'Z'));
}

static inline int
iswalpha (wint_t wc)
{
  return (wc & ~0x20) >= 'A' && (wc & ~0x20) <= 'Z';
}

static inline int
iswblank (wint_t wc)
{
  return wc == ' ' || wc == '\t';
}

static inline int
iswcntrl (wint_t wc)
{
  return (wc & ~0x1f) == 0 || wc == 0x7f;
}

static inline int
iswdigit (wint_t wc)
{
  return wc >= '0' && wc <= '9';
}

static inline int
iswgraph (wint_t wc)
{
  return wc >= '!' && wc <= '~';
}

static inline int
iswlower (wint_t wc)
{
  return wc >= 'a' && wc <= 'z';
}

static inline int
iswprint (wint_t wc)
{
  return wc >= ' ' && wc <= '~';
}

static inline int
iswpunct (wint_t wc)
{
  return (wc >= '!' && wc <= '~'
	  && !((wc >= '0' && wc <= '9')
	       || ((wc & ~0x20) >= 'A' && (wc & ~0x20) <= 'Z')));
}

static inline int
iswspace (wint_t wc)
{
  return (wc == ' ' || wc == '\t'
	  || wc == '\n' || wc == '\v' || wc == '\f' || wc == '\r');
}

static inline int
iswupper (wint_t wc)
{
  return wc >= 'A' && wc <= 'Z';
}

static inline int
iswxdigit (wint_t wc)
{
  return ((wc >= '0' && wc <= '9')
	  || ((wc & ~0x20) >= 'A' && (wc & ~0x20) <= 'F'));
}

static inline wint_t
towlower (wint_t wc)
{
  return (wc >= 'A' && wc <= 'Z' ? wc - 'A' + 'a' : wc);
}

static inline wint_t
towupper (wint_t wc)
{
  return (wc >= 'a' && wc <= 'z' ? wc - 'a' + 'A' : wc);
}

# endif /* ! HAVE_ISWCNTRL */

# if defined __MINGW32__

/* On native Windows, wchar_t is uint16_t, and wint_t is uint32_t.
   The functions towlower and towupper are implemented in the MSVCRT library
   to take a wchar_t argument and return a wchar_t result.  mingw declares
   these functions to take a wint_t argument and return a wint_t result.
   This means that:
   1. When the user passes an argument outside the range 0x0000..0xFFFF, the
      function will look only at the lower 16 bits.  This is allowed according
      to POSIX.
   2. The return value is returned in the lower 16 bits of the result register.
      The upper 16 bits are random: whatever happened to be in that part of the
      result register.  We need to fix this by adding a zero-extend from
      wchar_t to wint_t after the call.  */

static inline wint_t
rpl_towlower (wint_t wc)
{
  return (wint_t) (wchar_t) towlower (wc);
}
#  define towlower rpl_towlower

static inline wint_t
rpl_towupper (wint_t wc)
{
  return (wint_t) (wchar_t) towupper (wc);
}
#  define towupper rpl_towupper

# endif

#endif /* _GL_WCTYPE_H */
#endif /* _GL_WCTYPE_H */
