/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include "tty_private.h"

#include <stdlib.h>
#include <string.h>
#include <util/AutoLock.h>


//#define MASTER_TRACE
#ifdef MASTER_TRACE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


struct master_cookie : tty_cookie {
};


struct tty gMasterTTYs[kNumTTYs];


static status_t
master_service(struct tty *tty, uint32 op)
{
	// nothing here yet
	return B_OK;
}


static status_t
create_master_cookie(master_cookie *&cookie, struct tty *master,
	struct tty *slave, uint32 openMode)
{
	cookie = (master_cookie*)malloc(sizeof(struct master_cookie));
	if (cookie == NULL)
		return B_NO_MEMORY;

	status_t error = init_tty_cookie(cookie, master, slave, openMode);
	if (error != B_OK) {
		free(cookie);
		return error;
	}

	return B_OK;
}


//	#pragma mark -


static status_t
master_open(const char *name, uint32 flags, void **_cookie)
{
	bool findUnusedTTY = strcmp(name, "ptmx") == 0;

	int32 index = -1;
	if (!findUnusedTTY) {
		index = get_tty_index(name);
		if (index >= (int32)kNumTTYs)
			return B_ERROR;
	}

	TRACE(("master_open: TTY index = %" B_PRId32 " (name = %s)\n", index,
		name));

	MutexLocker globalLocker(gGlobalTTYLock);

	if (findUnusedTTY) {
		for (index = 0; index < (int32)kNumTTYs; index++) {
			if (gMasterTTYs[index].ref_count == 0)
				break;
		}
		if (index >= (int32)kNumTTYs)
			return ENOENT;
	} else if (gMasterTTYs[index].ref_count > 0) {
		// we're already open!
		return B_BUSY;
	}

	gMasterTTYs[index].opened_count = 0;
	gSlaveTTYs[index].opened_count = 0;
	status_t status = tty_open(&gMasterTTYs[index], &master_service);
	if (status < B_OK) {
		// initializing TTY failed
		return status;
	}

	master_cookie *cookie;
	status = create_master_cookie(cookie, &gMasterTTYs[index],
		&gSlaveTTYs[index], flags);
	if (status != B_OK) {
		tty_close(&gMasterTTYs[index]);
		return status;
	}

	add_tty_cookie(cookie);

	*_cookie = cookie;

	return B_OK;
}


static status_t
master_close(void *_cookie)
{
	master_cookie *cookie = (master_cookie *)_cookie;

	TRACE(("master_close: cookie %p\n", _cookie));

	MutexLocker globalLocker(gGlobalTTYLock);

	// close all connected slave cookies first
	while (tty_cookie *slave = cookie->other_tty->cookies.Head())
		tty_close_cookie(slave);

	// close the client cookie
	tty_close_cookie(cookie);

	return B_OK;
}


static status_t
master_free_cookie(void *_cookie)
{
	// The TTY is already closed. We only have to free the cookie.
	master_cookie *cookie = (master_cookie *)_cookie;

	MutexLocker globalLocker(gGlobalTTYLock);
	uninit_tty_cookie(cookie);
	globalLocker.Unlock();

	free(cookie);

	return B_OK;
}


static status_t
master_ioctl(void *_cookie, uint32 op, void *buffer, size_t length)
{
	master_cookie *cookie = (master_cookie *)_cookie;

	TRACE(("master_ioctl: cookie %p, op %" B_PRIu32 ", buffer %p, length %lu"
		"\n", _cookie, op, buffer, length));

	return tty_ioctl(cookie, op, buffer, length);
}


static status_t
master_read(void *_cookie, off_t offset, void *buffer, size_t *_length)
{
	master_cookie *cookie = (master_cookie *)_cookie;

	TRACE(("master_read: cookie %p, offset %" B_PRIdOFF ", buffer %p, length "
		"%lu\n", _cookie, offset, buffer, *_length));

	status_t result = tty_input_read(cookie, buffer, _length);

	TRACE(("master_read done: cookie %p, result: %" B_PRIx32 ", length %lu\n",
		_cookie, result, *_length));

	return result;
}


static status_t
master_write(void *_cookie, off_t offset, const void *buffer, size_t *_length)
{
	master_cookie *cookie = (master_cookie *)_cookie;

	TRACE(("master_write: cookie %p, offset %" B_PRIdOFF ", buffer %p, length "
		"%lu\n", _cookie, offset, buffer, *_length));

	status_t result = tty_write_to_tty_master(cookie, buffer, _length);

	TRACE(("master_write done: cookie %p, result: %" B_PRIx32 ", length %lu\n",
		_cookie, result, *_length));

	return result;
}


static status_t
master_select(void *_cookie, uint8 event, uint32 ref, selectsync *sync)
{
	master_cookie *cookie = (master_cookie *)_cookie;

	return tty_select(cookie, event, ref, sync);
}


static status_t
master_deselect(void *_cookie, uint8 event, selectsync *sync)
{
	master_cookie *cookie = (master_cookie *)_cookie;

	return tty_deselect(cookie, event, sync);
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
