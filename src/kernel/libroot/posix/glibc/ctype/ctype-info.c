/* Copyright (C) 1991, 92, 95, 96, 97, 99, 2000 Free Software Foundation, Inc.
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

#include <ctype.h>
#include <locale/localeinfo.h>

/* Defined in locale/C-ctype.c.  */
extern const char _nl_C_LC_CTYPE_class[];
extern const char _nl_C_LC_CTYPE_class32[];
extern const char _nl_C_LC_CTYPE_toupper[];
extern const char _nl_C_LC_CTYPE_tolower[];
extern const char _nl_C_LC_CTYPE_class_upper[];
extern const char _nl_C_LC_CTYPE_class_lower[];
extern const char _nl_C_LC_CTYPE_class_alpha[];
extern const char _nl_C_LC_CTYPE_class_digit[];
extern const char _nl_C_LC_CTYPE_class_xdigit[];
extern const char _nl_C_LC_CTYPE_class_space[];
extern const char _nl_C_LC_CTYPE_class_print[];
extern const char _nl_C_LC_CTYPE_class_graph[];
extern const char _nl_C_LC_CTYPE_class_blank[];
extern const char _nl_C_LC_CTYPE_class_cntrl[];
extern const char _nl_C_LC_CTYPE_class_punct[];
extern const char _nl_C_LC_CTYPE_class_alnum[];
extern const char _nl_C_LC_CTYPE_map_toupper[];
extern const char _nl_C_LC_CTYPE_map_tolower[];
extern const char _nl_C_LC_CTYPE_width[];

#define b(t,x,o) (((const t *) _nl_C_LC_CTYPE_##x) + o)

const unsigned short int *__ctype_b = b (unsigned short int, class, 128);
const __uint32_t *__ctype32_b = b (__uint32_t, class32, 0);
const __int32_t *__ctype_tolower = b (__int32_t, tolower, 128);
const __int32_t *__ctype_toupper = b (__int32_t, toupper, 128);
const __uint32_t *__ctype32_tolower = b (__uint32_t, tolower, 128);
const __uint32_t *__ctype32_toupper = b (__uint32_t, toupper, 128);
const char *__ctype32_wctype[12] =
{
  b(char, class_upper, 32),
  b(char, class_lower, 32),
  b(char, class_alpha, 32),
  b(char, class_digit, 32),
  b(char, class_xdigit, 32),
  b(char, class_space, 32),
  b(char, class_print, 32),
  b(char, class_graph, 32),
  b(char, class_blank, 32),
  b(char, class_cntrl, 32),
  b(char, class_punct, 32),
  b(char, class_alnum, 32)
};
const char *__ctype32_wctrans[2] =
{
  b(char, map_toupper, 0),
  b(char, map_tolower, 0)
};
const char *__ctype32_width = b (char, width, 0);
