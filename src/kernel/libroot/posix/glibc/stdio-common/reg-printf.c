/* Copyright (C) 1991, 1996, 1997, 2002 Free Software Foundation, Inc.
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
   02111-1307 USA.
*/


#include <stdio_private.h>
#include <errno.h>
#include <limits.h>
#include <printf.h>
#include <stddef.h>
#include <stdlib.h>


/* Array of functions indexed by format character.  */
libc_freeres_ptr (printf_arginfo_function **__printf_arginfo_table)
  attribute_hidden;
printf_function **__printf_function_table attribute_hidden;

int __register_printf_function __P ((int, printf_function,
                                     printf_arginfo_function));

/* Register FUNC to be called to format SPEC specifiers.  */
int
__register_printf_function (spec, converter, arginfo)
     int spec;
     printf_function converter;
     printf_arginfo_function arginfo;
{
  if (spec < 0 || spec > (int) UCHAR_MAX)
    {
      __set_errno (EINVAL);
      return -1;
    }

  if (__printf_function_table == NULL)
    {
      __printf_arginfo_table = (printf_arginfo_function **)
	malloc ((UCHAR_MAX + 1) * sizeof (void *) * 2);
      if (__printf_arginfo_table == NULL)
	return -1;
      __printf_function_table = (printf_function **)
	(__printf_arginfo_table + UCHAR_MAX + 1);
    }

  __printf_function_table[spec] = converter;
  __printf_arginfo_table[spec] = arginfo;

  return 0;
}
weak_alias (__register_printf_function, register_printf_function)
