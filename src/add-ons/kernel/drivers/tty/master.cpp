/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include "tty_private.h"

#include <stdlib.h>


#define MASTER_TRACE
#ifdef MASTER_TRACE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


struct master_cookie {
	struct tty	*tty;
	uint32		open_mode;
};


struct tty gMasterTTYs[kNumTTYs];


static status_t
master_open(const char *name, uint32 flags, void **_cookie)
{
	int32 index = get_tty_index(name);

	if (atomic_or(&gMasterTTYs[index].open_count, 1) != 0) {
		// we're already open!
		return B_BUSY;
	}

	master_cookie *cookie = (master_cookie *)malloc(sizeof(struct master_cookie));
	if (cookie == NULL)
		return B_NO_MEMORY;

	cookie->tty = &gMasterTTYs[index];
	*_cookie = cookie;

	return B_OK;
}


static status_t
master_close(void *_cookie)
{
	master_cookie *cookie = (master_cookie *)_cookie;

	TRACE(("master_close: cookie %p\n", _cookie));

	atomic_and(&cookie->tty->open_count, 0);
	return B_OK;
}


static status_t
master_free_cookie(void *_cookie)
{
	free(_cookie);
	return B_OK;
}


static status_t
master_ioctl(void *_cookie, uint32 op, void *buffer, size_t length)
{
	TRACE(("master_ioctl: cookie %p, op %lu, buffer %p, length %lu\n", _cookie, op, buffer, length));
	return B_BAD_VALUE;
}


static status_t
master_read(void *_cookie, off_t offset, void *buffer, size_t *_length)
{
	TRACE(("master_read: cookie %p, offset %Ld, buffer %p, length %lu\n", _cookie, offset, buffer, *_length));
	return B_BAD_VALUE;
}


static status_t
master_write(void *_cookie, off_t offset, const void *buffer, size_t *_length)
{
	TRACE(("master_write: cookie %p, offset %Ld, buffer %p, length %lu\n", _cookie, offset, buffer, *_length));
	return B_BAD_VALUE;
}


static status_t
master_select(void *_cookie, uint8 event, uint32 ref, selectsync *sync)
{
	return B_OK;
}


static status_t
master_deselect(void *_cookie, uint8 event, selectsync *sync)
{
	return B_OK;
}


device_hooks gMasterTTYHooks = {
	&master_open,
	&master_close,
	&master_free_cookie,
	&master_ioctl,
	&master_read,
	&master_write,
	&master_select,
	&master_deselect,
	NULL,	// read_pages()
	NULL	// write_pages()
};
