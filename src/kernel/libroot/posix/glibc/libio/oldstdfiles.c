/* Copyright (C) 1993,94,96,97,99,2000,2002 Free Software Foundation, Inc.
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

//#include <shlib-compat.h>
//#if SHLIB_COMPAT (libc, GLIBC_2_0, GLIBC_2_1)

/* This file provides definitions of _IO_stdin, _IO_stdout, and _IO_stderr
   for C code.  Compare stdstreams.cc.
   (The difference is that here the vtable field is set to 0,
   so the objects defined are not valid C++ objects.  On the other
   hand, we don't need a C++ compiler to build this file.) */

#define _IO_USE_OLD_IO_FILE
#include "libioP.h"

#ifdef _IO_MTSAFE_IO
#define DEF_STDFILE(NAME, FD, CHAIN, FLAGS) \
  static _IO_lock_t _IO_stdfile_##FD##_lock = _IO_lock_initializer; \
  struct _IO_FILE_plus NAME \
    = {FILEBUF_LITERAL(CHAIN, FLAGS, FD, NULL), &_IO_old_file_jumps};
#else
#define DEF_STDFILE(NAME, FD, CHAIN, FLAGS) \
  struct _IO_FILE_plus NAME \
    = {FILEBUF_LITERAL(CHAIN, FLAGS, FD, NULL), &_IO_old_file_jumps};
#endif

DEF_STDFILE(_IO_stdin_, 0, 0, _IO_NO_WRITES);
DEF_STDFILE(_IO_stdout_, 1, &_IO_stdin_, _IO_NO_READS);
DEF_STDFILE(_IO_stderr_, 2, &_IO_stdout_, _IO_NO_READS+_IO_UNBUFFERED);

#if defined __GNUC__ && __GNUC__ >= 2

#include <stdio.h>

//extern const int _IO_stdin_used;
//weak_extern (_IO_stdin_used);

#undef stdin
#undef stdout
#undef stderr

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

static void _IO_check_libio __P ((void)) __attribute__ ((constructor));

/* This function determines which shared C library the application
   was linked against. We then set up the stdin/stdout/stderr and
   _IO_list_all accordingly. */

static void
_IO_check_libio ()
{
	_kern_debug_output("Hey dude!\n");
  //if (&_IO_stdin_used == NULL)
    {
      /* We are using the old one. */
      _IO_stdin = stdin = (_IO_FILE *) &_IO_stdin_;
      _IO_stdout = stdout = (_IO_FILE *) &_IO_stdout_;
      _IO_stderr = stderr = (_IO_FILE *) &_IO_stderr_;
      INTUSE(_IO_list_all) = &_IO_stderr_;
      _IO_stdin->_vtable_offset = _IO_stdout->_vtable_offset =
	_IO_stderr->_vtable_offset = stdin->_vtable_offset =
	stdout->_vtable_offset = stderr->_vtable_offset =
	((int) sizeof (struct _IO_FILE)
	 - (int) sizeof (struct _IO_FILE_complete));
    }
}

#endif

//#endif
