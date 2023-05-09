/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2023, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <new>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <util/AutoLock.h>
#include <Drivers.h>

#include <team.h>

extern "C" {
#include <drivers/tty.h>
#include <tty_module.h>
}
#include "tty_private.h"


//#define PTY_TRACE
#ifdef PTY_TRACE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif

#define DRIVER_NAME "pty"


int32 api_version = B_CUR_DRIVER_API_VERSION;
tty_module_info *gTTYModule = NULL;

struct mutex gGlobalTTYLock;

static const uint32 kNumTTYs = 64;
char *gDeviceNames[kNumTTYs * 2 + 3];
	// reserve space for "pt/" and "tt/" entries, "ptmx", "tty",
	// and the terminating NULL

struct tty* gMasterTTYs[kNumTTYs];
struct tty* gSlaveTTYs[kNumTTYs];

extern device_hooks gMasterPTYHooks, gSlavePTYHooks;


status_t
init_hardware(void)
{
	TRACE((DRIVER_NAME ": init_hardware()\n"));
	return B_OK;
}


status_t
init_driver(void)
{
	status_t status = get_module(B_TTY_MODULE_NAME, (module_info **)&gTTYModule);
	if (status < B_OK)
		return status;

	TRACE((DRIVER_NAME ": init_driver()\n"));

	mutex_init(&gGlobalTTYLock, "tty global");
	memset(gDeviceNames, 0, sizeof(gDeviceNames));
	memset(gMasterTTYs, 0, sizeof(gMasterTTYs));
	memset(gSlaveTTYs, 0, sizeof(gSlaveTTYs));

	// create driver name array

	char letter = 'p';
	int8 digit = 0;

	for (uint32 i = 0; i < kNumTTYs; i++) {
		// For compatibility, we have to create the same mess in /dev/pt and
		// /dev/tt as BeOS does: we publish devices p0, p1, ..., pf, r1, ...,
		// sf. It would be nice if we could drop compatibility and create
		// something better. In fact we already don't need the master devices
		// anymore, since "/dev/ptmx" does the job. The slaves entries could
		// be published on the fly when a master is opened (e.g via
		// vfs_create_special_node()).
		char buffer[64];

		snprintf(buffer, sizeof(buffer), "pt/%c%x", letter, digit);
		gDeviceNames[i] = strdup(buffer);

		snprintf(buffer, sizeof(buffer), "tt/%c%x", letter, digit);
		gDeviceNames[i + kNumTTYs] = strdup(buffer);

		if (++digit > 15)
			digit = 0, letter++;

		if (!gDeviceNames[i] || !gDeviceNames[i + kNumTTYs]) {
			uninit_driver();
			return B_NO_MEMORY;
		}
	}

	gDeviceNames[2 * kNumTTYs] = (char *)"ptmx";
	gDeviceNames[2 * kNumTTYs + 1] = (char *)"tty";

	return B_OK;
}


void
uninit_driver(void)
{
	TRACE((DRIVER_NAME ": uninit_driver()\n"));

	for (int32 i = 0; i < (int32)kNumTTYs * 2; i++)
		free(gDeviceNames[i]);

	mutex_destroy(&gGlobalTTYLock);

	put_module(B_TTY_MODULE_NAME);
}


const char **
publish_devices(void)
{
	TRACE((DRIVER_NAME ": publish_devices()\n"));
	return const_cast<const char **>(gDeviceNames);
}


device_hooks *
find_device(const char *name)
{
	TRACE((DRIVER_NAME ": find_device(\"%s\")\n", name));

	for (uint32 i = 0; gDeviceNames[i] != NULL; i++) {
		if (!strcmp(name, gDeviceNames[i])) {
			return i < kNumTTYs || i == (2 * kNumTTYs)
				? &gMasterPTYHooks : &gSlavePTYHooks;
		}
	}

	return NULL;
}


static int32
get_tty_index(const char *name)
{
	// device names follow this form: "pt/%c%x"
	int8 digit = name[4];
	if (digit >= 'a') {
		// hexadecimal digits
		digit -= 'a' - 10;
	} else
		digit -= '0';

	return (name[3] - 'p') * 16 + digit;
}


static int32
get_tty_index(struct tty *tty)
{
	int32 index = -1;
	for (uint32 i = 0; i < kNumTTYs; i++) {
		if (tty == gMasterTTYs[i] || tty == gSlaveTTYs[i]) {
			index = i;
			break;
		}
	}
	return index;
}


//	#pragma mark - device hooks


static bool
master_service(struct tty *tty, uint32 op, void *buffer, size_t length)
{
	// nothing here yet
	return false;
}


static bool
slave_service(struct tty *tty, uint32 op, void *buffer, size_t length)
{
	// nothing here yet
	return false;
}


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

	TRACE(("pty_open: TTY index = %" B_PRId32 " (name = %s)\n", index, name));

	MutexLocker globalLocker(gGlobalTTYLock);

	if (findUnusedTTY) {
		for (index = 0; index < (int32)kNumTTYs; index++) {
			if (gMasterTTYs[index] == NULL || gMasterTTYs[index]->ref_count == 0)
				break;
		}
		if (index >= (int32)kNumTTYs)
			return ENOENT;
	} else if (gMasterTTYs[index] != NULL && gMasterTTYs[index]->ref_count != 0) {
		// we're already open!
		return B_BUSY;
	}

	status_t status = B_OK;

	if (gMasterTTYs[index] == NULL) {
		status = gTTYModule->tty_create(master_service, NULL, &gMasterTTYs[index]);
		if (status != B_OK)
			return status;
	}
	if (gSlaveTTYs[index] == NULL) {
		status = gTTYModule->tty_create(slave_service, gMasterTTYs[index], &gSlaveTTYs[index]);
		if (status != B_OK)
			return status;
	}

	tty_cookie *cookie;
	status = gTTYModule->tty_create_cookie(gMasterTTYs[index], gSlaveTTYs[index], flags, &cookie);
	if (status != B_OK)
		return status;

	*_cookie = cookie;
	return B_OK;
}


static status_t
slave_open(const char *name, uint32 flags, void **_cookie)
{
	// Get the tty index: Opening "/dev/tty" means opening the process'
	// controlling tty.
	int32 index = get_tty_index(name);
	if (strcmp(name, "tty") == 0) {
		struct tty *controllingTTY = (struct tty *)team_get_controlling_tty();
		if (controllingTTY == NULL)
			return B_NOT_ALLOWED;

		index = get_tty_index(controllingTTY);
		if (index < 0)
			return B_NOT_ALLOWED;
	} else {
		index = get_tty_index(name);
		if (index >= (int32)kNumTTYs)
			return B_ERROR;
	}

	TRACE(("slave_open: TTY index = %" B_PRId32 " (name = %s)\n", index,
		name));

	MutexLocker globalLocker(gGlobalTTYLock);

	// we may only be used if our master has already been opened
	if (gMasterTTYs[index] == NULL || gMasterTTYs[index]->open_count == 0
			|| gSlaveTTYs[index] == NULL) {
		return B_IO_ERROR;
	}

	bool makeControllingTTY = (flags & O_NOCTTY) == 0;
	pid_t processID = getpid();
	pid_t sessionID = getsid(processID);

	if (gSlaveTTYs[index]->open_count == 0) {
		// We only allow session leaders to open the tty initially.
		if (makeControllingTTY && processID != sessionID)
			return B_NOT_ALLOWED;
	} else if (makeControllingTTY) {
		// If already open, we allow only processes from the same session
		// to open the tty again while becoming controlling tty
		pid_t ttySession = gSlaveTTYs[index]->settings->session_id;
		if (ttySession >= 0) {
			makeControllingTTY = false;
		} else {
			// The tty is not associated with a session yet. The process needs
			// to be a session leader.
			if (makeControllingTTY && processID != sessionID)
				return B_NOT_ALLOWED;
		}
	}

	if (gSlaveTTYs[index]->open_count == 0) {
		gSlaveTTYs[index]->settings->session_id = -1;
		gSlaveTTYs[index]->settings->pgrp_id = -1;
	}

	tty_cookie *cookie;
	status_t status = gTTYModule->tty_create_cookie(gSlaveTTYs[index], gMasterTTYs[index], flags,
		&cookie);
	if (status != B_OK)
		return status;

	if (makeControllingTTY) {
		gSlaveTTYs[index]->settings->session_id = sessionID;
		gSlaveTTYs[index]->settings->pgrp_id = sessionID;
		team_set_controlling_tty(gSlaveTTYs[index]);
	}

	*_cookie = cookie;
	return B_OK;
}


static status_t
pty_close(void *_cookie)
{
	tty_cookie *cookie = (tty_cookie *)_cookie;

	TRACE(("pty_close: cookie %p\n", _cookie));

	MutexLocker globalLocker(gGlobalTTYLock);

	gTTYModule->tty_close_cookie(cookie);

	return B_OK;
}


static status_t
pty_free_cookie(void *_cookie)
{
	// The TTY is already closed. We only have to free the cookie.
	tty_cookie *cookie = (tty_cookie *)_cookie;

	gTTYModule->tty_destroy_cookie(cookie);

	return B_OK;
}


static status_t
pty_ioctl(void *_cookie, uint32 op, void *buffer, size_t length)
{
	tty_cookie *cookie = (tty_cookie *)_cookie;

	struct tty* tty = cookie->tty;
	RecursiveLocker locker(tty->lock);

	TRACE(("pty_ioctl: cookie %p, op %" B_PRIu32 ", buffer %p, length %lu"
		"\n", _cookie, op, buffer, length));

	switch (op) {
		case B_IOCTL_GET_TTY_INDEX:
		{
			int32 ptyIndex = get_tty_index(cookie->tty);
			if (ptyIndex < 0)
				return B_BAD_VALUE;

			if (user_memcpy(buffer, &ptyIndex, sizeof(int32)) < B_OK)
				return B_BAD_ADDRESS;

			return B_OK;
		}

		case B_IOCTL_GRANT_TTY:
		{
			if (!cookie->tty->is_master)
				return B_BAD_VALUE;

			int32 ptyIndex = get_tty_index(cookie->tty);
			if (ptyIndex < 0)
				return B_BAD_VALUE;

			// get slave path
			char path[64];
			snprintf(path, sizeof(path), "/dev/%s",
				gDeviceNames[kNumTTYs + ptyIndex]);

			// set owner and permissions respectively
			if (chown(path, getuid(), getgid()) != 0
				|| chmod(path, S_IRUSR | S_IWUSR | S_IWGRP) != 0) {
				return errno;
			}

			return B_OK;
		}

		case 'pgid':				// BeOS
			op = TIOCSPGRP;

		case 'wsiz':				// BeOS
			op = TIOCSWINSZ;
			break;

		default:
			break;
	}

	return gTTYModule->tty_control(cookie, op, buffer, length);
}


static status_t
pty_read(void *_cookie, off_t offset, void *buffer, size_t *_length)
{
	tty_cookie *cookie = (tty_cookie *)_cookie;

	TRACE(("pty_read: cookie %p, offset %" B_PRIdOFF ", buffer %p, length "
		"%lu\n", _cookie, offset, buffer, *_length));

	status_t result = gTTYModule->tty_read(cookie, buffer, _length);

	TRACE(("pty_read done: cookie %p, result: %" B_PRIx32 ", length %lu\n",
		_cookie, result, *_length));

	return result;
}


static status_t
pty_write(void *_cookie, off_t offset, const void *buffer, size_t *_length)
{
	tty_cookie *cookie = (tty_cookie *)_cookie;

	TRACE(("pty_write: cookie %p, offset %" B_PRIdOFF ", buffer %p, length "
		"%lu\n", _cookie, offset, buffer, *_length));

	status_t result = gTTYModule->tty_write(cookie, buffer, _length);

	TRACE(("pty_write done: cookie %p, result: %" B_PRIx32 ", length %lu\n",
		_cookie, result, *_length));

	return result;
}


static status_t
pty_select(void *_cookie, uint8 event, uint32 ref, selectsync *sync)
{
	tty_cookie *cookie = (tty_cookie *)_cookie;

	return gTTYModule->tty_select(cookie, event, ref, sync);
}


static status_t
pty_deselect(void *_cookie, uint8 event, selectsync *sync)
{
	tty_cookie *cookie = (tty_cookie *)_cookie;

	return gTTYModule->tty_deselect(cookie, event, sync);
}


device_hooks gMasterPTYHooks = {
	&master_open,
	&pty_close,
	&pty_free_cookie,
	&pty_ioctl,
	&pty_read,
	&pty_write,
	&pty_select,
	&pty_deselect,
	NULL,	// read_pages()
	NULL	// write_pages()
};

device_hooks gSlavePTYHooks = {
	&slave_open,
	&pty_close,
	&pty_free_cookie,
	&pty_ioctl,
	&pty_read,
	&pty_write,
	&pty_select,
	&pty_deselect,
	NULL,	// read_pages()
	NULL	// write_pages()
};
