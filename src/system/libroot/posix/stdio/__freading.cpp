/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <libio.h>
#include <stdio_ext.h>


int
__freading(FILE* stream)
{
	// Return true, if writing is not allowed or the last operation was a read.
	return (stream->_flags & _IO_NO_WRITES) != 0
		|| ((stream->_flags & (_IO_NO_READS | _IO_CURRENTLY_PUTTING)) == 0
			&& stream->_IO_read_base != NULL);
}
