/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

#define _BUILDING_fs 1

//#define TEST_RB

#include <sys/param.h>
#ifndef TEST_RB
//#include "lock.h"
#include "googlefs.h"
#endif

#ifdef TEST_RB
#include <OS.h>
struct ring_buffer {
        size_t size;
        size_t current; /* index of next byte to read */
        size_t avail;   /* number of bytes in */
        unsigned char data[0];
};
#define ASSERT(op) if (!(op)) debugger("ASSERT: " #op " in " __FILE__ ":" __FUNCTION__)
#else
#define ASSERT(op) if (!(op)) panic("ASSERT: %s in %s:%s", #op, __FILE__, __FUNCTION__)
#endif

void rb_init(struct ring_buffer *rb, size_t size)
{
	rb->size = size;
	rb->current = 0;
	rb->avail = 0;
}

void rb_clear(struct ring_buffer *rb)
{
	rb->avail = 0;
	rb->current = 0;
}

size_t rb_can_write(struct ring_buffer *rb)
{
	if (!rb)
		return 0;
	return (rb->size - rb->avail);
}

size_t rb_can_read(struct ring_buffer *rb)
{
	if (!rb)
		return 0;
	return rb->avail;
}

size_t rb_write(struct ring_buffer *rb, void *data, size_t len)
{
	size_t index, towrite, written;
	if (!rb)
		return 0;
	index = (rb->current + rb->avail) % rb->size;
	towrite = rb_can_write(rb);
	towrite = MIN(len, towrite);
	if (towrite < 1)
		return 0;
	len = rb->size - index;
	len = MIN(len, towrite);
	memcpy(((char *)rb->data)+index, data, len);
	rb->avail += len;
	written = len;
	if (len < towrite) {
		towrite -= len;
		len = MIN(towrite, rb_can_write(rb));
		index = 0;
		memcpy(((char *)rb->data)+index, ((char *)data)+written, len);
		rb->avail += len;
		written += len;
	}
	ASSERT(rb->avail <= rb->size);
	return written;
}

size_t rb_read(struct ring_buffer *rb, void *data, size_t len)
{
	size_t index, toread, got;
	if (!rb)
		return 0;
	index = rb->current;
	toread = rb_can_read(rb);
	toread = MIN(len, toread);
	if (toread < 1)
		return 0;
	len = rb->size - index;
	len = MIN(len, toread);
	memcpy(data, ((char *)rb->data)+index, len);
	rb->avail -= len;
	rb->current += len;
	got = len;
	if (len < toread) {
		toread -= len;
		len = MIN(toread, rb_can_read(rb));
		index = 0;
		memcpy(((char *)data)+got, ((char *)rb->data)+index, len);
		rb->current = len;
		rb->avail -= len;
		got += len;
	}
	ASSERT(rb->avail <= rb->size);
	return got;
}

#ifdef TEST_RB
int main(void)
{
	struct _myrb {
		struct ring_buffer rb;
		char buff[10];
	} myrb;
	char buffer[10];
	char obuffer[10];
	int got, tow;
	rb_init(&myrb.rb, 10);
	while ((got = read(0, buffer, 10))) {
		int len, olen;
		len = rb_write(&myrb.rb, buffer, got);
		olen = rb_read(&myrb.rb, obuffer, 10);
		write(1, obuffer, olen);
	}
	return 1;
}
#endif


