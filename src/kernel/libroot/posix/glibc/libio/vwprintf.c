/* Copyright (C) 1991,1993,1995,1997,1999,2001 Free Software Foundation, Inc.
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

#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>

/* Write formatted output to stdout according to the
   format string FORMAT, using the argument list in ARG.  */
int
vwprintf (format, arg)
     const wchar_t *format;
     __gnuc_va_list arg;
{
  return __vfwprintf (stdout, format, arg);
}
