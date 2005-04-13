/* Copyright (C) 1996, 1997, 1999, 2000, 2001 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1996.

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

#include <wchar.h>
#include "../locale/coll-lookup.h"

#define STRING_TYPE wchar_t
#define USTRING_TYPE wint_t
#ifdef USE_IN_EXTENDED_LOCALE_MODEL
# define STRCOLL __wcscoll_l
#else
# define STRCOLL __wcscoll
#endif
#define STRCMP wcscmp
#define STRLEN __wcslen
#define WEIGHT_H "../locale/weightwc.h"
#define SUFFIX	WC
#define L(arg) L##arg
#define WIDE_CHAR_VERSION 1

#include "../string/strcoll.c"

#ifndef USE_IN_EXTENDED_LOCALE_MODEL
weak_alias (__wcscoll, wcscoll)
libc_hidden_weak (wcscoll)
#endif
