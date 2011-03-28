/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "packet_buffer.h"

#include <util/ring_buffer.h>

#include <stdlib.h>
#include <string.h>


packet_buffer*
create_packet_buffer(size_t size)
{
	packet_buffer* buffer
		= (packet_buffer *)malloc(sizeof(packet_buffer));
	if (buffer == NULL)
		return NULL;

	buffer->buffer = create_ring_buffer(size);
	if (buffer->buffer == NULL) {
		free(buffer);
		return NULL;
	}
	B_INITIALIZE_SPINLOCK(&buffer->lock);

	return buffer;
}


void
delete_packet_buffer(packet_buffer* buffer)
{
	delete_ring_buffer(buffer->buffer);
	free(buffer);
}


void
packet_buffer_clear(packet_buffer* buffer)
{
	cpu_status state = disable_interrupts();
	acquire_spinlock(&buffer->lock);

	ring_buffer_clear(buffer->buffer);

	release_spinlock(&buffer->lock);
	restore_interrupts(state);
}


size_t
packet_buffer_readable(packet_buffer* buffer)
{
	cpu_status state = disable_interrupts();
	acquire_spinlock(&buffer->lock);

	size_t available = ring_buffer_readable(buffer->buffer);

	release_spinlock(&buffer->lock);
	restore_interrupts(state);

	return available;
}


size_t
packet_buffer_writable(packet_buffer* buffer)
{
	cpu_status state = disable_interrupts();
	acquire_spinlock(&buffer->lock);

	size_t left = ring_buffer_writable(buffer->buffer);

	release_spinlock(&buffer->lock);
	restore_interrupts(state);

	return left;
}


void
packet_buffer_flush(packet_buffer* buffer, size_t length)
{
	cpu_status state = disable_interrupts();
	acquire_spinlock(&buffer->lock);

	ring_buffer_flush(buffer->buffer, length);

	release_spinlock(&buffer->lock);
	restore_interrupts(state);
}


size_t
packet_buffer_read(packet_buffer* buffer, uint8 *data, size_t length)
{
	cpu_status state = disable_interrupts();
	acquire_spinlock(&buffer->lock);

	size_t bytesRead = ring_buffer_read(buffer->buffer, data, length);

	release_spinlock(&buffer->lock);
	restore_interrupts(state);

	return bytesRead;
}


size_t
packet_buffer_write(packet_buffer* buffer, const uint8 *data,
	size_t length)
{
	cpu_status state = disable_interrupts();
	acquire_spinlock(&buffer->lock);

	size_t bytesWritten = ring_buffer_write(buffer->buffer, data, length);

	release_spinlock(&buffer->lock);
	restore_interrupts(state);

	return bytesWritten;
}

