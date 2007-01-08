/* Create a unique temporary file.

   Copyright (C) 2006 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* written by Jim Meyering */

#include <stdlib.h>
#include <unistd.h>

#ifdef __MKSTEMP_PREFIX
# define _GL_CONCAT(x, y) x ## y
# define _GL_XCONCAT(x, y) _GL_CONCAT (x, y)
# define __MKSTEMP_ID(y) _GL_XCONCAT (__MKSTEMP_PREFIX, y)
# undef mkstemp
# define mkstemp __MKSTEMP_ID (mkstemp)
int mkstemp (char *);
#endif
