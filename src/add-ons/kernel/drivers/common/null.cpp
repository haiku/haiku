/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <Drivers.h>
#include <KernelExport.h>
#include <string.h>


#define DEVICE_NAME_FULL "full"
#define DEVICE_NAME_NULL "null"
#define DEVICE_NAME_ZERO "zero"

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
zero_read(void *cookie, off_t pos, void *buffer, size_t *_length)
{
	if (user_memset(buffer, 0, *_length) < B_OK)
		return B_BAD_ADDRESS;

	return B_OK;
}


static status_t
null_write(void *cookie, off_t pos, const void *buffer, size_t *_length)
{
	return B_OK;
}


static status_t
full_write(void *cookie, off_t pos, const void *buffer, size_t *_length)
{
	return B_DEVICE_FULL;
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
		DEVICE_NAME_FULL,
		DEVICE_NAME_NULL,
		DEVICE_NAME_ZERO,
		NULL
	};

	return devices;
}


device_hooks *
find_device(const char *name)
{
	static device_hooks fullHooks = {
		&null_open,
		&null_close,
		&null_freecookie,
		&null_ioctl,
		&null_read,
		&full_write,
	};

	static device_hooks nullHooks = {
		&null_open,
		&null_close,
		&null_freecookie,
		&null_ioctl,
		&null_read,
		&null_write,
	};

	static device_hooks zeroHooks = {
		&null_open,
		&null_close,
		&null_freecookie,
		&null_ioctl,
		&zero_read,
		&null_write,
	};


	if (strcmp(name, DEVICE_NAME_FULL) == 0)
		return &fullHooks;
	if (strcmp(name, DEVICE_NAME_NULL) == 0)
		return &nullHooks;
	if (strcmp(name, DEVICE_NAME_ZERO) == 0)
		return &zeroHooks;

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

