/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <Drivers.h>
#include <KernelExport.h>

#include <string.h>


#define DEVICE_NAME "zero"

int32 api_version = B_CUR_DRIVER_API_VERSION;


static status_t
zero_open(const char *name, uint32 flags, void **cookie)
{
	*cookie = NULL;
	return B_OK;
}


static status_t
zero_close(void *cookie)
{
	return B_OK;
}


static status_t
zero_freecookie(void *cookie)
{
	return B_OK;
}


static status_t
zero_ioctl(void *cookie, uint32 op, void *buffer, size_t length)
{
	return EPERM;
}


static status_t
zero_read(void *cookie, off_t pos, void *buffer, size_t *_length)
{
	if (user_memset(buffer, 0, *_length) < B_OK)
		return B_BAD_ADDRESS;

	return B_OK;
}


static status_t
zero_write(void *cookie, off_t pos, const void *buffer, size_t *_length)
{
	return B_OK;
}


//	#pragma mark -


status_t
init_hardware(void)
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


status_t
init_driver(void)
{
	return B_OK;
}


void
uninit_driver(void)
{
}

