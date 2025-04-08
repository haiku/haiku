/* Copyright (C) 1996-2014 Free Software Foundation, Inc.
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

#include "libioP.h"


#undef _IO_flockfile
#undef _IO_funlockfile


void
_IO_flockfile(_IO_FILE *stream)
{
	_IO_lock_lock (*stream->_lock);
}

void
_IO_funlockfile(_IO_FILE *stream)
{
	_IO_lock_unlock (*stream->_lock);
}

int
_IO_ftrylockfile(_IO_FILE *stream)
{
	return _IO_lock_trylock (*stream->_lock);
}

weak_alias (_IO_flockfile, flockfile);
weak_alias (_IO_funlockfile, funlockfile);
weak_alias (_IO_ftrylockfile, ftrylockfile);
