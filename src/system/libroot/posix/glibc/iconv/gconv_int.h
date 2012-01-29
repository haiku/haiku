/* Copyright (C) 1997,1998,1999,2000,2001,2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1997.

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

#ifndef _GCONV_INT_H
#define _GCONV_INT_H	1

#include "gconv.h"
#include <stdlib.h>		/* For alloca used in macro below.  */

__BEGIN_DECLS


/* How many character should be conveted in one call?  */
#define GCONV_NCHAR_GOAL	8160


/* Description for an available conversion module.  */
struct gconv_module
{
  const char *from_string;
  const char *to_string;

  int cost_hi;
  int cost_lo;

  const char *module_name;

  struct gconv_module *left;	/* Prefix smaller.  */
  struct gconv_module *same;	/* List of entries with identical prefix.  */
  struct gconv_module *right;	/* Prefix larger.  */
};


/* Internal data structure to represent transliteration module.  */
struct trans_struct
{
  const char *name;
  struct trans_struct *next;

  const char **csnames;
  size_t ncsnames;
  __gconv_trans_fct trans_fct;
  __gconv_trans_context_fct trans_context_fct;
  __gconv_trans_init_fct trans_init_fct;
  __gconv_trans_end_fct trans_end_fct;
};


/* Builtin transformations.  */
#ifdef _LIBC
# define __BUILTIN_TRANSFORM(Name) \
  extern int Name (struct __gconv_step *step,				      \
		   struct __gconv_step_data *data,			      \
		   const unsigned char **inbuf,				      \
		   const unsigned char *inbufend,			      \
		   unsigned char **outbufstart, size_t *irreversible,	      \
		   int do_flush, int consume_incomplete)

__BUILTIN_TRANSFORM (__gconv_transform_multibyte_wchar);
__BUILTIN_TRANSFORM (__gconv_transform_wchar_multibyte);
# undef __BUITLIN_TRANSFORM

#endif

__END_DECLS

#endif /* gconv_int.h */
