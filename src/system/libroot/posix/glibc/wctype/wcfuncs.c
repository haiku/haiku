/* Copyright (C) 1996,1997,1998,1999,2000,2001 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <wctype.h>
#include <locale/localeinfo.h>

#include "wchar-lookup.h"

/* These are not exported.  */
extern const char *__ctype32_wctype[12];
extern const char *__ctype32_wctrans[2];

/* Provide real-function versions of all the wctype macros.  */

#define	func(name, type)					\
  extern int __##name (wint_t __wc);				\
  int								\
  __##name (wint_t wc)						\
  {								\
    return wctype_table_lookup (__ctype32_wctype[type], wc);	\
  }								\
  weak_alias (__##name, name)

#undef iswalnum
func (iswalnum, __ISwalnum)
#undef iswalpha
func (iswalpha, __ISwalpha)
#undef iswblank
func (iswblank, __ISwblank)
#undef iswcntrl
func (iswcntrl, __ISwcntrl)
#undef iswdigit
func (iswdigit, __ISwdigit)
#undef iswlower
func (iswlower, __ISwlower)
#undef iswgraph
func (iswgraph, __ISwgraph)
#undef iswprint
func (iswprint, __ISwprint)
#undef iswpunct
func (iswpunct, __ISwpunct)
#undef iswspace
func (iswspace, __ISwspace)
#undef iswupper
func (iswupper, __ISwupper)
#undef iswxdigit
func (iswxdigit, __ISwxdigit)

wint_t
(towlower) (wc)
     wint_t wc;
{
  return wctrans_table_lookup (__ctype32_wctrans[__TOW_tolower], wc);
}

wint_t
(towupper) (wc)
     wint_t wc;
{
  return wctrans_table_lookup (__ctype32_wctrans[__TOW_toupper], wc);
}
