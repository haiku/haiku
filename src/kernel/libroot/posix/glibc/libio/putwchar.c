/* Copyright (C) 1991, 95, 96, 97, 98, 99 Free Software Foundation, Inc.
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

#include "libioP.h"
#include <wchar.h>

wint_t
putwchar (wc)
     wchar_t wc;
{
  wint_t result;
  _IO_cleanup_region_start ((void (*) __P ((void *))) _IO_funlockfile,
			    _IO_stdout);
  _IO_flockfile (_IO_stdout);
  result = _IO_putwc_unlocked (wc, _IO_stdout);
  _IO_funlockfile (_IO_stdout);
  _IO_cleanup_region_end (0);
  return result;
}
