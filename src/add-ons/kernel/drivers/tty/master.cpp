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
	struct tty	*slave;
	uint32		open_mode;
};


struct tty gMasterTTYs[kNumTTYs];


static status_t
master_service(struct tty *tty, uint32 op)
{
	// nothing here yet
	return B_OK;
}


//	#pragma mark -


static status_t
master_open(const char *name, uint32 flags, void **_cookie)
{
	int32 index = get_tty_index(name);
	dprintf("TTY index = %ld (name = %s)\n", index, name);

	if (atomic_or(&gMasterTTYs[index].open_count, 1) != 0) {
		// we're already open!
		return B_BUSY;
	}

	status_t status = tty_open(&gMasterTTYs[index], &master_service);
	if (status < B_OK) {
		// initializing TTY failed, reset open counter
		atomic_and(&gMasterTTYs[index].open_count, 0);
		return status;
	}

	master_cookie *cookie = (master_cookie *)malloc(sizeof(struct master_cookie));
	if (cookie == NULL) {
		atomic_and(&gMasterTTYs[index].open_count, 0);
		tty_close(&gMasterTTYs[index]);
		return B_NO_MEMORY;
	}

	cookie->tty = &gMasterTTYs[index];
	cookie->slave = &gSlaveTTYs[index];
	cookie->open_mode = flags;
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
	master_cookie *cookie = (master_cookie *)_cookie;

	tty_close(cookie->tty);
	free(cookie);
	return B_OK;
}


static status_t
master_ioctl(void *_cookie, uint32 op, void *buffer, size_t length)
{
	master_cookie *cookie = (master_cookie *)_cookie;

	TRACE(("master_ioctl: cookie %p, op %lu, buffer %p, length %lu\n", _cookie, op, buffer, length));
	return tty_ioctl(cookie->tty, op, buffer, length);
}


static status_t
master_read(void *_cookie, off_t offset, void *buffer, size_t *_length)
{
	master_cookie *cookie = (master_cookie *)_cookie;

	TRACE(("master_read: cookie %p, offset %Ld, buffer %p, length %lu\n", _cookie, offset, buffer, *_length));
	return tty_input_read(cookie->tty, buffer, _length, cookie->open_mode);
}


static status_t
master_write(void *_cookie, off_t offset, const void *buffer, size_t *_length)
{
	master_cookie *cookie = (master_cookie *)_cookie;

	TRACE(("master_write: cookie %p, offset %Ld, buffer %p, length %lu\n", _cookie, offset, buffer, *_length));
	return tty_write_to_tty(cookie->tty, cookie->slave, buffer, _length, cookie->open_mode, true);
}


static status_t
master_select(void *_cookie, uint8 event, uint32 ref, selectsync *sync)
{
	master_cookie *cookie = (master_cookie *)_cookie;

	return tty_select(cookie->tty, event, ref, sync);
}


static status_t
master_deselect(void *_cookie, uint8 event, selectsync *sync)
{
	master_cookie *cookie = (master_cookie *)_cookie;

	return tty_deselect(cookie->tty, event, sync);
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
