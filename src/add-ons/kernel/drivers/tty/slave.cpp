/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <stdlib.h>

#include <util/AutoLock.h>

#include "tty_private.h"


//#define SLAVE_TRACE
#ifdef SLAVE_TRACE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif


struct slave_cookie : tty_cookie {
};


struct tty gSlaveTTYs[kNumTTYs];


static status_t
slave_open(const char *name, uint32 flags, void **_cookie)
{
	int32 index = get_tty_index(name);
	if (index >= (int32)kNumTTYs)
		return B_ERROR;

	TRACE(("slave_open: TTY index = %ld (name = %s)\n", index, name));

	MutexLocker globalLocker(gGlobalTTYLock);

	// we may only be used if our master has already been opened
	if (gMasterTTYs[index].open_count == 0)
		return B_IO_ERROR;

	if (gSlaveTTYs[index].open_count == 0) {
		status_t status = tty_open(&gSlaveTTYs[index], NULL);
		if (status < B_OK) {
			// initializing TTY failed
			return status;
		}
	}

 	slave_cookie *cookie = (slave_cookie *)malloc(sizeof(struct slave_cookie));
	if (cookie == NULL) {
		if (gSlaveTTYs[index].open_count == 0)
			tty_close(&gSlaveTTYs[index]);

		return B_NO_MEMORY;
	}

	status_t status = init_tty_cookie(cookie, &gSlaveTTYs[index],
		&gMasterTTYs[index], flags);
	if (status != B_OK) {
		free(cookie);

		if (gSlaveTTYs[index].open_count == 0)
			tty_close(&gSlaveTTYs[index]);

		return status;
	}

	if (gSlaveTTYs[index].open_count == 0)
		gSlaveTTYs[index].lock = gMasterTTYs[index].lock;

	add_tty_cookie(cookie);

	*_cookie = cookie;

	return B_OK;
}


static status_t
slave_close(void *_cookie)
{
	slave_cookie *cookie = (slave_cookie *)_cookie;

	TRACE(("slave_close: cookie %p\n", _cookie));

	MutexLocker globalLocker(gGlobalTTYLock);

	// unblock and wait for all blocking operations
	tty_close_cookie(cookie);

	return B_OK;
}


static status_t
slave_free_cookie(void *_cookie)
{
	// The TTY is already closed. We only have to free the cookie.
	slave_cookie *cookie = (slave_cookie *)_cookie;

	TRACE(("slave_free_cookie: cookie %p\n", _cookie));

	uninit_tty_cookie(cookie);
	free(cookie);

	return B_OK;
}


static status_t
slave_ioctl(void *_cookie, uint32 op, void *buffer, size_t length)
{
	slave_cookie *cookie = (slave_cookie *)_cookie;

	TRACE(("slave_ioctl: cookie %p, op %lu, buffer %p, length %lu\n", _cookie, op, buffer, length));

	return tty_ioctl(cookie, op, buffer, length);		
}


static status_t
slave_read(void *_cookie, off_t offset, void *buffer, size_t *_length)
{
	slave_cookie *cookie = (slave_cookie *)_cookie;

	TRACE(("slave_read: cookie %p, offset %Ld, buffer %p, length %lu\n",
		_cookie, offset, buffer, *_length));

	status_t result = tty_input_read(cookie, buffer, _length);

	TRACE(("slave_read done: cookie %p, result %lx, length %lu\n",
		_cookie, result, *_length));

	return result;
}


static status_t
slave_write(void *_cookie, off_t offset, const void *buffer, size_t *_length)
{
	slave_cookie *cookie = (slave_cookie *)_cookie;

	TRACE(("slave_write: cookie %p, offset %Ld, buffer %p, length %lu\n",
		_cookie, offset, buffer, *_length));

	status_t result = tty_write_to_tty(cookie, buffer, _length, false);

	TRACE(("slave_write done: cookie %p, result %lx, length %lu\n",
		_cookie, result, *_length));

	return result;
}


static status_t
slave_select(void *_cookie, uint8 event, uint32 ref, selectsync *sync)
{
	slave_cookie *cookie = (slave_cookie *)_cookie;

	return tty_select(cookie, event, ref, sync);
}


static status_t
slave_deselect(void *_cookie, uint8 event, selectsync *sync)
{
	slave_cookie *cookie = (slave_cookie *)_cookie;

	return tty_deselect(cookie, event, sync);
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
