/*
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2016, Jessica Hamilton, jessica.l.hamilton@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include "device.h"

#include <stdlib.h>
#include <string.h>

#include <Drivers.h>
#include <graphic_driver.h>
#include <image.h>
#include <KernelExport.h>
#include <OS.h>
#include <PCI.h>
#include <SupportDefs.h>

#include <vesa.h>

#include "driver.h"
#include "utility.h"
#include "vesa_info.h"
#include "framebuffer_private.h"


//#define TRACE_DEVICE
#ifdef TRACE_DEVICE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


static status_t
device_open(const char* name, uint32 flags, void** _cookie)
{
	int id;

	// find accessed device
	char* thisName;

	// search for device name
	for (id = 0; (thisName = gDeviceNames[id]) != NULL; id++) {
		if (!strcmp(name, thisName))
			break;
	}
	if (thisName == NULL)
		return B_BAD_VALUE;

	framebuffer_info* info = gDeviceInfo[id];

	mutex_lock(&gLock);

	status_t status = B_OK;

	if (info->open_count == 0) {
		// this device has been opened for the first time, so
		// we allocate needed resources and initialize the structure
		if (status == B_OK)
			status = framebuffer_init(*info);
		if (status == B_OK)
			info->id = id;
	}

	if (status == B_OK) {
		info->open_count++;
		*_cookie = info;
	}

	mutex_unlock(&gLock);
	return status;
}


static status_t
device_close(void* cookie)
{
	return B_OK;
}


static status_t
device_free(void* cookie)
{
	struct framebuffer_info* info = (framebuffer_info*)cookie;

	mutex_lock(&gLock);

	if (info->open_count-- == 1) {
		// release info structure
		framebuffer_uninit(*info);
	}

	mutex_unlock(&gLock);
	return B_OK;
}


static status_t
device_ioctl(void* cookie, uint32 msg, void* buffer, size_t bufferLength)
{
	struct framebuffer_info* info = (framebuffer_info*)cookie;

	switch (msg) {
		case B_GET_ACCELERANT_SIGNATURE:
			dprintf(DEVICE_NAME ": acc: %s\n", ACCELERANT_NAME);
			if (user_strlcpy((char*)buffer, ACCELERANT_NAME,
					B_FILE_NAME_LENGTH) < B_OK)
				return B_BAD_ADDRESS;

			return B_OK;

		// needed to share data between kernel and accelerant
		case VESA_GET_PRIVATE_DATA:
			return user_memcpy(buffer, &info->shared_area, sizeof(area_id));

		// needed for cloning
		case VESA_GET_DEVICE_NAME:
			if (user_strlcpy((char*)buffer, gDeviceNames[info->id],
					B_PATH_NAME_LENGTH) < B_OK)
				return B_BAD_ADDRESS;

			return B_OK;

		default:
			TRACE((DEVICE_NAME ": ioctl() unknown message %ld (length = %lu)\n",
				msg, bufferLength));
			break;
	}

	return B_DEV_INVALID_IOCTL;
}


static status_t
device_read(void* /*cookie*/, off_t /*pos*/, void* /*buffer*/, size_t* _length)
{
	*_length = 0;
	return B_NOT_ALLOWED;
}


static status_t
device_write(void* /*cookie*/, off_t /*pos*/, const void* /*buffer*/,
	size_t* _length)
{
	*_length = 0;
	return B_NOT_ALLOWED;
}


device_hooks gDeviceHooks = {
	device_open,
	device_close,
	device_free,
	device_ioctl,
	device_read,
	device_write,
	NULL,
	NULL,
	NULL,
	NULL
};
