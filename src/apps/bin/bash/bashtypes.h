/* bashtypes.h -- Bash system types. */

/* Copyright (C) 1993 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2, or (at your option) any later
   version.

   Bash is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License along
   with Bash; see the file COPYING.  If not, write to the Free Software
   Foundation, 59 Temple Place, Suite 330, Boston, MA 02111 USA. */

#if !defined (_BASHTYPES_H_)
#  define _BASHTYPES_H_

#if defined (CRAY)
#  define word __word
#endif

#include <sys/types.h>

#if defined (CRAY)
#  undef word
#endif

#if defined (HAVE_INTTYPES_H)
#  include <inttypes.h>
#endif

#endif /* _BASHTYPES_H_ */
