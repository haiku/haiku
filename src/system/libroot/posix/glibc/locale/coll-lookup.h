/* Copyright (C) 2000, 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Bruno Haible <haible@clisp.cons.org>, 2000.

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

#include <stdint.h>

/* Lookup in a table of int32_t, with default value 0.  */
extern int32_t __collidx_table_lookup (const char *table, uint32_t wc)
     internal_function;

/* Lookup in a table of uint32_t, with default value 0xffffffff.  */
extern uint32_t __collseq_table_lookup (const char *table, uint32_t wc)
     internal_function;
