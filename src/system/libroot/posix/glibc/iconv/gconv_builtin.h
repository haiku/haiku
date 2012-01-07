/* Builtin transformations.
   Copyright (C) 1997-1999, 2000-2002 Free Software Foundation, Inc.
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

BUILTIN_TRANSFORMATION ("MULTIBYTE", "WCHAR", 1, "=multibyte->wchar",
			__gconv_transform_multibyte_wchar, NULL, 4, 4, 1, MB_LEN_MAX)

BUILTIN_TRANSFORMATION ("WCHAR", "MULTIBYTE", 1, "=wchar->multibyte",
			__gconv_transform_wchar_multibyte, NULL, 4, 4, 1, MB_LEN_MAX)
