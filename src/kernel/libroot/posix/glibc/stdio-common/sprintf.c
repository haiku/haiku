/* Copyright (C) 1991, 1995, 1997, 1998, 2002 Free Software Foundation, Inc.
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

#ifdef USE_IN_LIBIO
# include <libio/iolibio.h>
# define vsprintf(s, f, a) INTUSE(_IO_vsprintf) (s, f, a)
#endif

/* Write formatted output into S, according to the format string FORMAT.  */
/* VARARGS2 */
int
sprintf (char *s, const char *format, ...)
{
  va_list arg;
  int done;

  va_start (arg, format);
  done = vsprintf (s, format, arg);
  va_end (arg);

  return done;
}
libc_hidden_def (sprintf)

#ifdef USE_IN_LIBIO
strong_alias(sprintf, _IO_sprintf)
#endif
