/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include "line_buffer.h"

#include <KernelExport.h>
#include <stdlib.h>


status_t
clear_line_buffer(struct line_buffer &buffer)
{
	buffer.in = 0;
	buffer.first = 0;
}


status_t
init_line_buffer(struct line_buffer &buffer, size_t size)
{
	clear_line_buffer(buffer);
	
	buffer.buffer = (char *)malloc(size);
	if (buffer.buffer == NULL)
		return B_NO_MEMORY;

	buffer.size = size;

	return B_OK;
}


status_t
uninit_line_buffer(struct line_buffer &buffer)
{
	free(buffer.buffer);
	return B_OK;
}


int32
line_buffer_readable(struct line_buffer &buffer)
{
	return buffer.in;
}


int32
line_buffer_writable(struct line_buffer &buffer)
{
	return buffer.size - buffer.in;
}


ssize_t
line_buffer_user_read(struct line_buffer &buffer, char *data, size_t length)
{
	size_t available = buffer.in;

	if (length > available)
		length = available;

	if (length == 0)
		return 0;

	ssize_t bytesRead = length;

	if (buffer.first + length < buffer.size) {
		// simple copy
		if (user_memcpy(data, buffer.buffer + buffer.first, length) != B_OK)
			bytesRead = B_BAD_ADDRESS;
	} else {
		// need to copy both ends
		size_t upper = buffer.size - buffer.first;
		size_t lower = length - upper;

		if (user_memcpy(data, buffer.buffer + buffer.first, upper) != B_OK
			|| user_memcpy(data + upper, buffer.buffer, lower) != B_OK)
			bytesRead = B_BAD_ADDRESS;
	}

	if (bytesRead > 0) {
		buffer.first = (buffer.first + bytesRead) % buffer.size;
		buffer.in -= bytesRead;
	}

	return bytesRead;
}


status_t
line_buffer_putc(struct line_buffer &buffer, char c)
{
	if (buffer.in == buffer.size)
		return B_NO_MEMORY;

	buffer.buffer[(buffer.first + buffer.in++) % buffer.size] = c;
}


#if 0
status_t
line_buffer_getc(struct line_buffer &buffer, char *_c)
{
}


status_t
line_buffer_ungetc(struct line_buffer &buffer, char *c)
{
}

#endif
