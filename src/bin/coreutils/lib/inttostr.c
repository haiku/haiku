/* inttostr.c -- convert integers to printable strings

   Copyright (C) 2001, 2006, 2008, 2009, 2010 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Written by Paul Eggert */

#include <config.h>

#include "inttostr.h"
#include "verify.h"

/* Convert I to a printable string in BUF, which must be at least
   INT_BUFSIZE_BOUND (INTTYPE) bytes long.  Return the address of the
   printable string, which need not start at BUF.  */

char *
inttostr (inttype i, char *buf)
{
  char *p = buf + INT_STRLEN_BOUND (inttype);
  *p = 0;

  { verify (TYPE_SIGNED (inttype) == inttype_is_signed); }
#if inttype_is_signed
  if (i < 0)
    {
      do
        *--p = '0' - i % 10;
      while ((i /= 10) != 0);

      *--p = '-';
    }
  else
#endif
    {
      do
        *--p = '0' + i % 10;
      while ((i /= 10) != 0);
    }

  return p;
}
