/* Copyright (C) 1991-2014 Free Software Foundation, Inc.
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
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include <stdarg.h>
#include <stdio.h>
#include <libioP.h>
#define __vsscanf(s, f, a) _IO_vsscanf (s, f, a)

/* Read formatted input from S, according to the format string FORMAT.  */
/* VARARGS2 */
int
sscanf (const char *s, const char *format, ...)
{
  va_list arg;
  int done;

  va_start (arg, format);
  done = __vsscanf (s, format, arg);
  va_end (arg);

  return done;
}
#undef _IO_sscanf
/* This is for libg++.  */
strong_alias (sscanf, _IO_sscanf)
