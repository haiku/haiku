/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel.h>
#include <stage2.h>
#include <memheap.h>
#include <devfs.h>
#include <vm.h>
#include <zero.h>
#include <string.h>
#include <Errors.h>

#define DEVICE_NAME "zero"

static int zero_open(const char *name, uint32 flags, void **cookie)
{
	*cookie = NULL;
	return 0;
}

static int zero_close(void * cookie)
{
	return 0;
}

static int zero_freecookie(void * cookie)
{
	return 0;
}

static int zero_ioctl(void * cookie, uint32 op, void *buf, size_t len)
{
	return EPERM;
}

static ssize_t zero_read(void * cookie, off_t pos, void *buf, size_t *len)
{
	int rc;

	rc = user_memset(buf, 0, *len);
	if(rc < 0)
		return rc;

	return 0;
}

static ssize_t zero_write(void * cookie, off_t pos, const void *buf, size_t *len)
{
	return 0;
}

status_t init_hardware()
{
	return 0;
}

const char **publish_devices(void)
{
	static const char *devices[] = {
		DEVICE_NAME, 
		NULL
	};

	return devices;
}

device_hooks *find_device(const char *name)
{
	static device_hooks hooks = {
		&zero_open,
		&zero_close,
		&zero_freecookie,
		&zero_ioctl,
		&zero_read,
		&zero_write,
		/* Leave select/deselect/readv/writev undefined. The kernel will
		 * use its own default implementation. The basic hooks above this
		 * line MUST be defined, however. */
		NULL,
		NULL,
		NULL,
		NULL
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


