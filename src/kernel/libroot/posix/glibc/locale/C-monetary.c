/* Copyright (C) 1995-1999, 2000, 2001, 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1995.

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

/* This table's entries are taken from POSIX.2 Table 2-9
   ``LC_MONETARY Category Definition in the POSIX Locale'',
   with additions from ISO 14652, section 4.4.  */
#ifdef __CHAR_UNSIGNED__
static const char not_available[] = "\377";
#else
static const char not_available[] = "\177";
#endif

const struct locale_data _nl_C_LC_MONETARY attribute_hidden =
{
  _nl_C_name,
  NULL, 0, 0,			/* no file mapped */
  { NULL, },			/* no cached data */
  UNDELETABLE,
  0,
  46,
  {
    { .string = "" },
    { .string = "" },
    { .string = "" },
    { .string = "" },
    { .string = "" },
    { .string = "" },
    { .string = "" },
    { .string = not_available },
    { .string = not_available },
    { .string = not_available },
    { .string = not_available },
    { .string = not_available },
    { .string = not_available },
    { .string = not_available },
    { .string = not_available },
    { .string = "-" },
    { .string = not_available },
    { .string = not_available },
    { .string = not_available },
    { .string = not_available },
    { .string = not_available },
    { .string = not_available },
    { .string = "" },
    { .string = "" },
    { .string = not_available },
    { .string = not_available },
    { .string = not_available },
    { .string = not_available },
    { .string = not_available },
    { .string = not_available },
    { .string = not_available },
    { .string = not_available },
    { .string = not_available },
    { .string = not_available },
    { .string = not_available },
    { .string = not_available },
    { .string = not_available },
    { .string = not_available },
    { .word = 10101 },
    { .word = 99991231 },
    { .word = 10101 },
    { .word = 99991231 },
    { .word = 1 },
    { .word = (unsigned int) L'\0' },
    { .word = (unsigned int) L'\0' },
    { .string = _nl_C_codeset }
  }
};
