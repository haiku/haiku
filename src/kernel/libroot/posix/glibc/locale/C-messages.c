/* Copyright (C) 1995,1996,1997,2000,2001,2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1995.

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

/* This table's entries are taken from POSIX.2 Table 2-12
   ``LC_MESSAGES Category Definition in the POSIX Locale''.

   The last two fields are a GNU extension.  */

const struct locale_data _nl_C_LC_MESSAGES attribute_hidden =
{
  _nl_C_name,
  NULL, 0, 0,			/* no file mapped */
  { NULL, },			/* no cached data */
  UNDELETABLE,
  0,
  5,
  {
    { .string = "^[yY]" },
    { .string = "^[nN]" },
    { .string = "" },
    { .string = "" },
    { .string = _nl_C_codeset }
  }
};
