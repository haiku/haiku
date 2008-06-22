/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <Drivers.h>
#include <string.h>


#define DEVICE_NAME "null"

int32 api_version = B_CUR_DRIVER_API_VERSION;


static status_t
null_open(const char *name, uint32 flags, void **cookie)
{
	*cookie = NULL;
	return B_OK;
}


static status_t
null_close(void *cookie)
{
	return B_OK;
}


static status_t
null_freecookie(void *cookie)
{
	return B_OK;
}


static status_t
null_ioctl(void *cookie, uint32 op, void *buffer, size_t length)
{
	return EPERM;
}


static status_t
null_read(void *cookie, off_t pos, void *buffer, size_t *_length)
{
	*_length = 0;
	return B_OK;
}


static status_t
null_write(void *cookie, off_t pos, const void *buffer, size_t *_length)
{
	return B_OK;
}


//	#pragma mark -


status_t
init_hardware()
{
	return B_OK;
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
	return B_OK;
}


void
uninit_driver(void)
{
}

