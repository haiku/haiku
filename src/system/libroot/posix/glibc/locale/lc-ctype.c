/* Define current locale data for LC_CTYPE category.
   Copyright (C) 1995-1999, 2000 Free Software Foundation, Inc.
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

#include "localeinfo.h"
#include <ctype.h>
#include <endian.h>
#include <stdint.h>

_NL_CURRENT_DEFINE (LC_CTYPE);

/* We are called after loading LC_CTYPE data to load it into
   the variables used by the ctype.h macros.

   There are three arrays of short ints which need to be indexable
   from -128 to 255 inclusive.  Stored in the locale data file are
   a copy of each for each byte order.  */

void
_nl_postload_ctype (void)
{
#define paste(a,b) paste1(a,b)
#define paste1(a,b) a##b

#define current(type,x,offset) \
  ((const type *) _NL_CURRENT (LC_CTYPE, paste(_NL_CTYPE_,x)) + offset)

  extern const uint32_t *__ctype32_b;
  extern const uint32_t *__ctype32_toupper;
  extern const uint32_t *__ctype32_tolower;
  extern const char *__ctype32_wctype[12];
  extern const char *__ctype32_wctrans[2];
  extern const char *__ctype32_width;

  size_t offset, cnt;

  __ctype_b = current (uint16_t, CLASS, 128);
  __ctype_toupper = current (uint32_t, TOUPPER, 128);
  __ctype_tolower = current (uint32_t, TOLOWER, 128);
  __ctype32_b = current (uint32_t, CLASS32, 0);
  __ctype32_toupper = current (uint32_t, TOUPPER32, 0);
  __ctype32_tolower = current (uint32_t, TOLOWER32, 0);

  offset = _NL_CURRENT_WORD (LC_CTYPE, _NL_CTYPE_CLASS_OFFSET);
  for (cnt = 0; cnt < 12; cnt++)
    __ctype32_wctype[cnt] = _nl_current_LC_CTYPE->values[offset + cnt].string;

  offset = _NL_CURRENT_WORD (LC_CTYPE, _NL_CTYPE_MAP_OFFSET);
  for (cnt = 0; cnt < 2; cnt++)
    __ctype32_wctrans[cnt] = _nl_current_LC_CTYPE->values[offset + cnt].string;

  __ctype32_width = current (char, WIDTH, 0);
}
