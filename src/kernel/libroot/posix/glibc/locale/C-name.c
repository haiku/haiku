/* Copyright (C) 1998, 2000, 2001, 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1998.

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

#include <endian.h>

#include "localeinfo.h"

/* This table's entries are taken from ISO 14652, the table in section
   4.9 "LC_NAME".  */

const struct locale_data _nl_C_LC_NAME attribute_hidden =
{
  _nl_C_name,
  NULL, 0, 0,			/* no file mapped */
  { NULL, },			/* no cached data */
  UNDELETABLE,
  0,
  7,
  {
    { .string = "%p%t%g%t%m%t%f" },
    { .string = "" },
    { .string = "" },
    { .string = "" },
    { .string = "" },
    { .string = "" },
    { .string = _nl_C_codeset }
  }
};
