/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKET_BUFFER_H
#define PACKET_BUFFER_H


#include <KernelExport.h>
#include <OS.h>


struct ring_buffer;


/**	The idea behind this packet buffer is to have multi-threading safe
 *	implementation that can be used in interrupts on top of the
 *	ring_buffer implementation provided by the kernel.
 *	It uses a spinlock for synchronization.
 *
 *	IOW if you don't have such high restrictions in your environment,
 *	you better don't want to use it at all.
 */
struct packet_buffer {
	struct ring_buffer*	buffer;
	spinlock			lock;
};


struct packet_buffer* create_packet_buffer(size_t size);
void delete_packet_buffer(struct packet_buffer* buffer);

void packet_buffer_clear(struct packet_buffer* buffer);
size_t packet_buffer_readable(struct packet_buffer* buffer);
size_t packet_buffer_writable(struct packet_buffer *buffer);
void packet_buffer_flush(struct packet_buffer* buffer, size_t bytes);
size_t packet_buffer_read(struct packet_buffer* buffer, uint8* data, size_t
	length);
size_t packet_buffer_write(struct packet_buffer* buffer, const uint8* data,
	size_t length);

#endif	/* PACKET_BUFFER_H */
