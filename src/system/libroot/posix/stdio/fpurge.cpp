/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <libio.h>
#include <stdio_ext.h>
#include <stdlib.h>


int
fpurge(FILE* stream)
{
	// purge read and write buffers
	stream->_IO_read_end = stream->_IO_read_ptr;
	stream->_IO_write_ptr = stream->_IO_write_base;

	// free ungetc buffer
	if (stream->_IO_save_base != NULL) {
		free(stream->_IO_save_base);
		stream->_IO_save_base = NULL;
	}

	return 0;
}


void
__fpurge(FILE* stream)
{
	fpurge(stream);
}
