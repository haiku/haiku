/*
 * Copyright 2005-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "ring_buffer.h"

#include <KernelExport.h>
#if 0
#include <port.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>

#ifndef HAIKU_TARGET_PLATFORM_HAIKU
#define user_memcpy(x...) (memcpy(x), B_OK)
#endif

/*!	This is a light-weight ring_buffer implementation.
 *	It does not provide any locking - you are supposed to ensure thread-safety
 *	with the restrictions you choose. Unless you are passing in unsafe buffers,
 *	the functions are safe to be called with interrupts turned off as well (not
 *	the user functions).
 *	They also don't use malloc() or any kind of locking after initialization.
 */


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


struct ring_buffer*
create_ring_buffer(size_t size)
{
	return create_ring_buffer_etc(NULL, size, 0);
}


struct ring_buffer*
create_ring_buffer_etc(void* memory, size_t size, uint32 flags)
{
	if (memory == NULL) {
		ring_buffer* buffer = (ring_buffer*)malloc(sizeof(ring_buffer) + size);
		if (buffer == NULL)
			return NULL;

		buffer->size = size;
		ring_buffer_clear(buffer);

		return buffer;
	}

	size -= sizeof(ring_buffer);
	ring_buffer* buffer = (ring_buffer*)memory;

	buffer->size = size;
	if ((flags & RING_BUFFER_INIT_FROM_BUFFER) != 0
		&& (size_t)buffer->size == size
		&& buffer->in >= 0 && (size_t)buffer->in <= size
		&& buffer->first >= 0 && (size_t)buffer->first < size) {
		// structure looks valid
	} else
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


/*!	Reads data from the ring buffer, but doesn't remove the data from it.
	\param buffer The ring buffer.
	\param offset The offset relative to the beginning of the data in the ring
		buffer at which to start reading.
	\param data The buffer to which to copy the data.
	\param length The number of bytes to read at maximum.
	\return The number of bytes actually read from the buffer.
*/
size_t
ring_buffer_peek(struct ring_buffer* buffer, size_t offset, void* data,
	size_t length)
{
	size_t available = buffer->in;

	if (offset >= available || length == 0)
		return 0;

	if (offset + length > available)
		length = available - offset;

	if ((offset += buffer->first) >= (size_t)buffer->size)
		offset -= buffer->size;

	if (offset + length <= (size_t)buffer->size) {
		// simple copy
		memcpy(data, buffer->buffer + offset, length);
	} else {
		// need to copy both ends
		size_t upper = buffer->size - offset;
		size_t lower = length - upper;

		memcpy(data, buffer->buffer + offset, upper);
		memcpy((uint8*)data + upper, buffer->buffer, lower);
	}

	return length;
}


/*!	Returns iovecs describing the contents of the ring buffer.

	\param buffer The ring buffer.
	\param vecs Pointer to an iovec array with at least 2 elements to be filled
		in by the function.
	\return The number of iovecs the function has filled in to describe the
		contents of the ring buffer. \c 0, if empty, \c 2 at maximum.
*/
int32
ring_buffer_get_vecs(struct ring_buffer* buffer, struct iovec* vecs)
{
	if (buffer->in == 0)
		return 0;

	if (buffer->first + buffer->in <= buffer->size) {
		// one element
		vecs[0].iov_base = buffer->buffer + buffer->first;
		vecs[0].iov_len = buffer->in;
		return 1;
	}

	// two elements
	size_t upper = buffer->size - buffer->first;
	size_t lower = buffer->in - upper;

	vecs[0].iov_base = buffer->buffer + buffer->first;
	vecs[0].iov_len = upper;
	vecs[1].iov_base = buffer->buffer;
	vecs[1].iov_len = lower;

	return 2;
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
