/* Copyright (C) 1993,1994,1996,1997,2000,2002 Free Software Foundation, Inc.
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

   As a special exception, if you link the code in this file with
   files compiled with a GNU compiler to produce an executable,
   that does not cause the resulting executable to be covered by
   the GNU Lesser General Public License.  This exception does not
   however invalidate any other reasons why the executable file
   might be covered by the GNU Lesser General Public License.
   This exception applies to code released by its copyright holders
   in files containing the exception.  */

#include "libioP.h"
#include "stdio.h"

#undef stdin
#undef stdout
#undef stderr
_IO_FILE *stdin = (FILE *) &_IO_2_1_stdin_;
_IO_FILE *stdout = (FILE *) &_IO_2_1_stdout_;
_IO_FILE *stderr = (FILE *) &_IO_2_1_stderr_;

#undef _IO_stdin
#undef _IO_stdout
#undef _IO_stderr
#ifdef _LIBC
# define AL(name) AL2 (name, _IO_##name)
# if defined HAVE_VISIBILITY_ATTRIBUTE
#  define AL2(name, al) \
  extern __typeof (name) al __attribute__ ((alias (#name),                    \
                                            visibility ("hidden")))
# else
#  define AL2(name, al) \
  extern __typeof (name) al __attribute__ ((alias (#name)))
# endif
AL(stdin);
AL(stdout);
AL(stderr);
#endif
