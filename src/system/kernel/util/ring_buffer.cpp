/*
 * Copyright 2005-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ring_buffer.h"

#include <KernelExport.h>
#include <lock.h>
#include <port.h>

#include <stdlib.h>
#include <string.h>


/**	This is a light-weight ring_buffer implementation.
 *	It does not provide any locking - you are supposed to ensure thread-safety
 *	with the restrictions you choose. Unless you are passing in unsafe buffers,
 *	the functions are safe to be called with interrupts turned off as well (not
 *	the user functions).
 *	They also don't use malloc() or any kind of locking after initialization.
 */

struct ring_buffer {
	int32		first;
	int32		in;
	int32		size;
	uint8		buffer[0];
};


static inline int32
space_left_in_buffer(struct ring_buffer *buffer)
{
	return buffer->size - buffer->in;
}


static ssize_t
read_from_buffer(struct ring_buffer *buffer, uint8 *data, ssize_t length,
	bool user)
{
	int32 available = buffer->in;

	if (length > available)
		length = available;

	if (length == 0)
		return 0;

	ssize_t bytesRead = length;

	if (buffer->first + length <= buffer->size) {
		// simple copy
		if (user) {
			if (user_memcpy(data, buffer->buffer + buffer->first, length) < B_OK)
				return B_BAD_ADDRESS;
		} else
			memcpy(data, buffer->buffer + buffer->first, length);
	} else {
		// need to copy both ends
		size_t upper = buffer->size - buffer->first;
		size_t lower = length - upper;

		if (user) {
			if (user_memcpy(data, buffer->buffer + buffer->first, upper) < B_OK
				|| user_memcpy(data + upper, buffer->buffer, lower) < B_OK)
				return B_BAD_ADDRESS;
		} else {
			memcpy(data, buffer->buffer + buffer->first, upper);
			memcpy(data + upper, buffer->buffer, lower);
		}
	}

	buffer->first = (buffer->first + bytesRead) % buffer->size;
	buffer->in -= bytesRead;

	return bytesRead;
}


static ssize_t
write_to_buffer(struct ring_buffer *buffer, const uint8 *data, ssize_t length,
	bool user)
{
	int32 left = space_left_in_buffer(buffer);
	if (length > left)
		length = left;

	if (length == 0)
		return 0;

	ssize_t bytesWritten = length;
	int32 position = (buffer->first + buffer->in) % buffer->size;

	if (position + length <= buffer->size) {
		// simple copy
		if (user) {
			if (user_memcpy(buffer->buffer + position, data, length) < B_OK)
				return B_BAD_ADDRESS;
		} else
			memcpy(buffer->buffer + position, data, length);
	} else {
		// need to copy both ends
		size_t upper = buffer->size - position;
		size_t lower = length - upper;

		if (user) {
			if (user_memcpy(buffer->buffer + position, data, upper) < B_OK
				|| user_memcpy(buffer->buffer, data + upper, lower) < B_OK)
				return B_BAD_ADDRESS;
		} else {
			memcpy(buffer->buffer + position, data, upper);
			memcpy(buffer->buffer, data + upper, lower);
		}
	}

	buffer->in += bytesWritten;

	return bytesWritten;
}


//	#pragma mark -


struct ring_buffer *
create_ring_buffer(size_t size)
{
	struct ring_buffer *buffer = (ring_buffer *)malloc(sizeof(ring_buffer) + size);
	if (buffer == NULL)
		return NULL;

	buffer->size = size;
	ring_buffer_clear(buffer);

	return buffer;
}


void
delete_ring_buffer(struct ring_buffer *buffer)
{
	free(buffer);
}


void
ring_buffer_clear(struct ring_buffer *buffer)
{
	buffer->in = 0;
	buffer->first = 0;
}


size_t
ring_buffer_readable(struct ring_buffer *buffer)
{
	return buffer->in;
}


size_t
ring_buffer_writable(struct ring_buffer *buffer)
{
	return buffer->size - buffer->in;
}


void
ring_buffer_flush(struct ring_buffer *buffer, size_t length)
{
	// we can't flush more bytes than there are
	if (length > (size_t)buffer->in)
		length = buffer->in;

	buffer->in -= length;
	buffer->first = (buffer->first + length) % buffer->size;
}


size_t
ring_buffer_read(struct ring_buffer *buffer, uint8 *data, ssize_t length)
{
	return read_from_buffer(buffer, data, length, false);
}


size_t
ring_buffer_write(struct ring_buffer *buffer, const uint8 *data, ssize_t length)
{
	return write_to_buffer(buffer, data, length, false);
}


ssize_t
ring_buffer_user_read(struct ring_buffer *buffer, uint8 *data, ssize_t length)
{
	return read_from_buffer(buffer, data, length, true);
}


ssize_t
ring_buffer_user_write(struct ring_buffer *buffer, const uint8 *data, ssize_t length)
{
	return write_to_buffer(buffer, data, length, true);
}


#if 0
/**	Sends the contents of the ring buffer to a port.
 *	The buffer will be empty afterwards only if sending the data actually works.
 */

status_t
ring_buffer_write_to_port(struct ring_buffer *buffer, port_id port, int32 code,
	uint32 flags, bigtime_t timeout)
{
	int32 length = buffer->in;
	if (length == 0)
		return B_OK;

	status_t status;

	if (buffer->first + length <= buffer->size) {
		// simple write
		status = write_port_etc(port, code, buffer->buffer + buffer->first, length,
			flags, timeout);
	} else {
		// need to write both ends
		size_t upper = buffer->size - buffer->first;
		size_t lower = length - upper;

		iovec vecs[2];
		vecs[0].iov_base = buffer->buffer + buffer->first;
		vecs[0].iov_len = upper;
		vecs[1].iov_base = buffer->buffer;
		vecs[1].iov_len = lower;

		status = writev_port_etc(port, code, vecs, 2, length, flags, timeout);
	}

	if (status < B_OK)
		return status;

	buffer->first = (buffer->first + length) % buffer->size;
	buffer->in -= length;

	return status;
}
#endif
