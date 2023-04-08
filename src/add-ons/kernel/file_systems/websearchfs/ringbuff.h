/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */
#ifndef _RINGBUFF_H
#define _RINGBUFF_H

struct ring_buffer {
	size_t size;
	size_t current; /* index of next byte to read */
	size_t avail;   /* number of bytes in */
	unsigned char data[0];
};

void rb_init(struct ring_buffer *rb, size_t size);
void rb_clear(struct ring_buffer *rb);
size_t rb_can_write(struct ring_buffer *rb);
size_t rb_can_read(struct ring_buffer *rb);
size_t rb_write(struct ring_buffer *rb, void *data, size_t len);
size_t rb_read(struct ring_buffer *rb, void *data, size_t len);

#endif
