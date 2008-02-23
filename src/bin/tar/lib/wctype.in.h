/* A substitute for ISO C99 <wctype.h>, for platforms that lack it.

   Copyright (C) 2006, 2007 Free Software Foundation, Inc.

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

/* Written by Bruno Haible and Paul Eggert.  */

/*
 * ISO C 99 <wctype.h> for platforms that lack it.
 * <http://www.opengroup.org/susv3xbd/wctype.h.html>
 *
 * iswctype, towctrans, towlower, towupper, wctrans, wctype,
 * wctrans_t, and wctype_t are not yet implemented.
 */

#ifndef _GL_WCTYPE_H

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

#if @HAVE_WINT_T@
typedef wint_t __wctype_wint_t;
#else
typedef int __wctype_wint_t;
#endif

/* FreeBSD 4.4 to 4.11 has <wctype.h> but lacks the functions.
   Assume all 12 functions are implemented the same way, or not at all.  */
#if ! @HAVE_ISWCNTRL@

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

static inline int
iswalnum (__wctype_wint_t wc)
{
  return ((wc >= '0' && wc <= '9')
	  || ((wc & ~0x20) >= 'A' && (wc & ~0x20) <= 'Z'));
}

static inline int
iswalpha (__wctype_wint_t wc)
{
  return (wc & ~0x20) >= 'A' && (wc & ~0x20) <= 'Z';
}

static inline int
iswblank (__wctype_wint_t wc)
{
  return wc == ' ' || wc == '\t';
}

static inline int
iswcntrl (__wctype_wint_t wc)
{
  return (wc & ~0x1f) == 0 || wc == 0x7f;
}

static inline int
iswdigit (__wctype_wint_t wc)
{
  return wc >= '0' && wc <= '9';
}

static inline int
iswgraph (__wctype_wint_t wc)
{
  return wc >= '!' && wc <= '~';
}

static inline int
iswlower (__wctype_wint_t wc)
{
  return wc >= 'a' && wc <= 'z';
}

static inline int
iswprint (__wctype_wint_t wc)
{
  return wc >= ' ' && wc <= '~';
}

static inline int
iswpunct (__wctype_wint_t wc)
{
  return (wc >= '!' && wc <= '~'
	  && !((wc >= '0' && wc <= '9')
	       || ((wc & ~0x20) >= 'A' && (wc & ~0x20) <= 'Z')));
}

static inline int
iswspace (__wctype_wint_t wc)
{
  return (wc == ' ' || wc == '\t'
	  || wc == '\n' || wc == '\v' || wc == '\f' || wc == '\r');
}

static inline int
iswupper (__wctype_wint_t wc)
{
  return wc >= 'A' && wc <= 'Z';
}

static inline int
iswxdigit (__wctype_wint_t wc)
{
  return ((wc >= '0' && wc <= '9')
	  || ((wc & ~0x20) >= 'A' && (wc & ~0x20) <= 'F'));
}

# endif /* ! HAVE_ISWCNTRL */

#endif /* _GL_WCTYPE_H */
#endif /* _GL_WCTYPE_H */
