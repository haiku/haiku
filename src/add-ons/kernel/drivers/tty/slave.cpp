/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include "tty_private.h"

#include <stdlib.h>


#define SLAVE_TRACE
#ifdef SLAVE_TRACE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif


struct slave_cookie {
	struct tty	*tty;
	uint32		open_mode;
};


struct tty gSlaveTTYs[kNumTTYs];


static status_t
slave_open(const char *name, uint32 flags, void **_cookie)
{
	int32 index = get_tty_index(name);

	if (gSlaveTTYs[index].open_count == 0)
		return B_IO_ERROR;

	atomic_add(&gSlaveTTYs[index].open_count, 1);

 	slave_cookie *cookie = (slave_cookie *)malloc(sizeof(struct slave_cookie));
	if (cookie == NULL)
		return B_NO_MEMORY;

	cookie->tty = &gSlaveTTYs[index];
	*_cookie = cookie;

	return B_OK;
}


static status_t
slave_close(void *_cookie)
{
	slave_cookie *cookie = (slave_cookie *)_cookie;

	TRACE(("slave_close: cookie %p\n", _cookie));

	atomic_add(&cookie->tty->open_count, -1);
	return B_OK;
}


static status_t
slave_free_cookie(void *_cookie)
{
	free(_cookie);
	return B_OK;
}


static status_t
slave_ioctl(void *_cookie, uint32 op, void *buffer, size_t length)
{
	TRACE(("slave_ioctl: cookie %p, op %lu, buffer %p, length %lu\n", _cookie, op, buffer, length));
	return B_BAD_VALUE;
}


static status_t
slave_read(void *_cookie, off_t offset, void *buffer, size_t *_length)
{
	TRACE(("slave_read: cookie %p, offset %Ld, buffer %p, length %lu\n", _cookie, offset, buffer, *_length));
	return B_BAD_VALUE;
}


static status_t
slave_write(void *_cookie, off_t offset, const void *buffer, size_t *_length)
{
	TRACE(("slave_write: cookie %p, offset %Ld, buffer %p, length %lu\n", _cookie, offset, buffer, *_length));
	return B_BAD_VALUE;
}


static status_t
slave_select(void *_cookie, uint8 event, uint32 ref, selectsync *sync)
{
	return B_OK;
}


static status_t
slave_deselect(void *_cookie, uint8 event, selectsync *sync)
{
	return B_OK;
}


device_hooks gSlaveTTYHooks = {
	&slave_open,
	&slave_close,
	&slave_free_cookie,
	&slave_ioctl,
	&slave_read,
	&slave_write,
	&slave_select,
	&slave_deselect,
	NULL,	// read_pages()
	NULL	// write_pages()
};
