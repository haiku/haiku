/*******************************************************************************
/
/	File:			driver.c
/
/   Description:	The digit driver allows a client to read 16 bytes of data.
/                   Each byte has the same value (by default 0); the client
/                   can change this value with write or ioctl commands.
/
/	Copyright 1999, Be Incorporated.   All Rights Reserved.
/	This file may be used under the terms of the Be Sample Code License.
/
*******************************************************************************/

#include <ktypes.h>
#include <drivers.h>
#include <debug.h>
#include <memheap.h>
#include <string.h>
#include <errno.h>

#define DPRINTF(a) dprintf("digit: "); dprintf a

#define MAX_SIZE 16
#define DEVICE_NAME "misc/digit"

#define TOUCH(x) ((void)x)

/*********************************/

struct digit_state {
	uchar value;
};

static status_t
digit_open(const char *name, uint32 mode, void **cookie)
{
	struct digit_state *s;

	TOUCH(name); TOUCH(mode);

	DPRINTF(("open\n"));

	s = (struct digit_state*) kmalloc(sizeof(*s));
	if (!s)
		return ENOMEM;

	s->value = 0;

	*cookie = s;

	return 0;
}

static status_t
digit_close(void *cookie)
{
	DPRINTF(("close\n"));

	TOUCH(cookie);

	return 0;
}

static status_t
digit_free(void *cookie)
{
	DPRINTF(("free\n"));

	kfree(cookie);

	return 0;
}

static status_t
digit_ioctl(void *cookie, uint32 op, void *data, size_t len)
{
	TOUCH(len);

	DPRINTF(("ioctl\n"));

	if (op == 'set ') {
		struct digit_state *s = (struct digit_state *)cookie;
		s->value = *(uchar *)data;
		DPRINTF(("Set digit to %2.2x\n", s->value));
		return 0;
	}

	return ENOSYS;
}

static ssize_t
digit_read(void *cookie, off_t pos, void *buffer, size_t *len)
{
	struct digit_state *s = (struct digit_state *)cookie;

	DPRINTF(("read\n"));

	if (pos >= MAX_SIZE) {
		*len = 0;
		return ESPIPE;
	} else if (pos + *len > MAX_SIZE)
		*len = (size_t)(MAX_SIZE - pos);

	if (*len)
		memset(buffer, s->value, *len);

	return 0;
}

static ssize_t
digit_write(void *cookie, off_t pos, const void *buffer, size_t *len)
{
	struct digit_state *s = (struct digit_state *)cookie;

	TOUCH(pos);

	s->value = ((uchar *)buffer)[*len - 1];

	DPRINTF(("write: set digit to %2.2x\n", s->value));

	return 0;
}

/***************************/

status_t init_hardware()
{
	DPRINTF(("Do you digit?\n"));
	return 0;
}

const char **publish_devices(void)
{
	static const char *devices[] = {
		DEVICE_NAME, NULL
	};

	return devices;
}

device_hooks *find_device(const char *name)
{
	static device_hooks hooks = {
		&digit_open,
		&digit_close,
		&digit_free,
		&digit_ioctl,
		&digit_read,
		&digit_write,
		/* Leave select/deselect/readv/writev undefined. The kernel will
		 * use its own default implementation. The basic hooks above this
		 * line MUST be defined, however. */
		NULL,
		NULL,
//		NULL,
//		NULL
	};

	if (!strcmp(name, DEVICE_NAME))
		return &hooks;

	return NULL;
}

status_t init_driver()
{
	return 0;
}

void uninit_driver()
{
}
