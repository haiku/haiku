/* Copyright (C) 1993,95,97,98,99,2000,2001,2002 Free Software Foundation, Inc.

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
   in files containing the exception.
*/


#include <libioP.h>


struct _IO_jump_t _IO_wfile_jumps = {
	JUMP_INIT_DUMMY,
	NULL
};
INTVARDEF(_IO_wfile_jumps)

struct _IO_jump_t _IO_wfile_jumps_mmap = {
	JUMP_INIT_DUMMY,
	NULL
};

struct _IO_jump_t _IO_wfile_jumps_maybe_mmap = {
	JUMP_INIT_DUMMY,
	NULL
};
