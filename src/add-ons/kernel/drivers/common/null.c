/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <Drivers.h>
#include <string.h>

#define DEVICE_NAME "null"


static status_t
null_open(const char *name, uint32 flags, void **cookie)
{
	*cookie = NULL;
	return 0;
}


static status_t
null_close(void *cookie)
{
	return 0;
}


static status_t
null_freecookie(void *cookie)
{
	return 0;
}


static status_t
null_ioctl(void *cookie, uint32 op, void *buffer, size_t length)
{
	return EPERM;
}


static ssize_t
null_read(void *cookie, off_t pos, void *buffer, size_t *length)
{
	return 0;
}


static ssize_t
null_write(void * cookie, off_t pos, const void *buf, size_t *len)
{
	return 0;
}


//	#pragma mark -


status_t
init_hardware()
{
	return 0;
}


const char **
publish_devices(void)
{
	static const char *devices[] = {
		DEVICE_NAME, 
		NULL
	};

	return devices;
}


device_hooks *
find_device(const char *name)
{
	static device_hooks hooks = {
		&null_open,
		&null_close,
		&null_freecookie,
		&null_ioctl,
		&null_read,
		&null_write,
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


status_t
init_driver(void)
{
	return 0;
}


void
uninit_driver(void)
{
}

