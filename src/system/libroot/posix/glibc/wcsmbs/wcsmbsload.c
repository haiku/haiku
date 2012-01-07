/* Copyright (C) 1998, 1999, 2000, 2001 Free Software Foundation, Inc.
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

#include <limits.h>

#include <locale/localeinfo.h>
#include <wcsmbsload.h>
#include <iconv/gconv_int.h>


/* These are the descriptions for the default conversion functions.  */
static struct __gconv_step to_wc =
{
  .__shlib_handle = NULL,
  .__modname = NULL,
  .__counter = INT_MAX,
  .__from_name = (char *) "MULTIBYTE",
  .__to_name = (char *) "WCHAR",
  .__fct = __gconv_transform_multibyte_wchar,
  .__init_fct = NULL,
  .__end_fct = NULL,
  .__min_needed_from = 1,
  .__max_needed_from = MB_LEN_MAX,
  .__min_needed_to = 4,
  .__max_needed_to = 4,
  .__stateful = 0,
  .__data = NULL
};

static struct __gconv_step to_mb =
{
  .__shlib_handle = NULL,
  .__modname = NULL,
  .__counter = INT_MAX,
  .__from_name = (char *) "WCHAR",
  .__to_name = (char *) "MULTIBYTE",
  .__fct = __gconv_transform_wchar_multibyte,
  .__init_fct = NULL,
  .__end_fct = NULL,
  .__min_needed_from = 4,
  .__max_needed_from = 4,
  .__min_needed_to = 1,
  .__max_needed_to = MB_LEN_MAX,
  .__stateful = 0,
  .__data = NULL
};


/* For the default locale we only have to handle ANSI_X3.4-1968.  */
struct gconv_fcts __wcsmbs_gconv_fcts =
{
  .towc = &to_wc,
  .towc_nsteps = 1,
  .tomb = &to_mb,
  .tomb_nsteps = 1
};


/* Clone the current conversion function set.  */
void
internal_function
__wcsmbs_clone_conv (struct gconv_fcts *copy)
{
  /* Copy the data.  */
  *copy = __wcsmbs_gconv_fcts;
}
