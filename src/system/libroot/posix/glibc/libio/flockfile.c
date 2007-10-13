/* 
 * Copyright 2003-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "libioP.h"


#undef _IO_flockfile
#undef _IO_funlockfile


void
_IO_flockfile(_IO_FILE *stream)
{
	__libc_lock_lock_recursive(*stream->_lock);
}


void
_IO_funlockfile(_IO_FILE *stream)
{
	__libc_lock_unlock_recursive(*stream->_lock);
}


int
_IO_ftrylockfile(_IO_FILE *stream)
{
	// ToDo: that code has probably never been tested!
	//return __libc_lock_trylock_recursive(*stream->_lock);
	return 1;
}

weak_alias (_IO_flockfile, flockfile);
weak_alias (_IO_funlockfile, funlockfile);
weak_alias (_IO_ftrylockfile, ftrylockfile);
