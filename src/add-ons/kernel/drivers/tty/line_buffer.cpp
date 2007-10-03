/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "line_buffer.h"

#include <KernelExport.h>
#include <stdlib.h>


status_t
clear_line_buffer(struct line_buffer &buffer)
{
	buffer.in = 0;
	buffer.first = 0;
	return B_OK;
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
line_buffer_readable_line(struct line_buffer &buffer, char eol, char eof)
{
	size_t size = buffer.in;
	if (size == 0)
		return 0;

	// find EOL or EOF char
	for (size_t i = 0; i < size; i++) {
		char c = buffer.buffer[(buffer.first + i) % buffer.size];
		if (c == eol || c == '\n' || c == '\r' || c == eof)
			return i + 1;
	}

	// If the buffer is full, but doesn't contain a EOL or EOF, we report the
	// full size anyway, since otherwise the reader would wait forever.
	return buffer.in == buffer.size ? buffer.in : 0;
}


int32
line_buffer_writable(struct line_buffer &buffer)
{
	return buffer.size - buffer.in;
}


ssize_t
line_buffer_user_read(struct line_buffer &buffer, char *data, size_t length,
	char eof, bool* hitEOF)
{
	size_t available = buffer.in;

	if (length > available)
		length = available;

	if (length == 0)
		return 0;

	// check for EOF, if the caller asked us to
	if (hitEOF) {
		*hitEOF = false;
		for (size_t i = 0; i < available; i++) {
			char c = buffer.buffer[(buffer.first + i) % buffer.size];
			if (c == eof) {
				*hitEOF = true;
				length = i;
				break;
			}
		}
	}

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

	// dispose of EOF char
	if (hitEOF && *hitEOF) {
		buffer.first = (buffer.first + 1) % buffer.size;
		buffer.in--;
	}

	return bytesRead;
}


status_t
line_buffer_putc(struct line_buffer &buffer, char c)
{
	if (buffer.in == buffer.size)
		return B_NO_MEMORY;

	buffer.buffer[(buffer.first + buffer.in++) % buffer.size] = c;
	return B_OK;
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


bool
line_buffer_tail_getc(struct line_buffer &buffer, char *c)
{
	if (buffer.in == 0)
		return false;

	*c = buffer.buffer[(buffer.first + --buffer.in) % buffer.size];
	return true;	
}
