/*
 * Copyright 2005-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "device.h"
#include "driver.h"
#include "utility.h"
#include "vesa_info.h"
#include "vga.h"

#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <PCI.h>
#include <SupportDefs.h>
#include <graphic_driver.h>
#include <image.h>

#include <stdlib.h>
#include <string.h>


//#define TRACE_DEVICE
#ifdef TRACE_DEVICE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


/* device hooks prototypes */

static status_t device_open(const char *, uint32, void **);
static status_t device_close(void *);
static status_t device_free(void *);
static status_t device_ioctl(void *, uint32, void *, size_t);
static status_t device_read(void *, off_t, void *, size_t *);
static status_t device_write(void *, off_t, const void *, size_t *);


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


//	#pragma mark -
//	the device will be accessed through the following functions (a.k.a. device hooks)


static status_t
device_open(const char *name, uint32 flags, void **_cookie)
{
	int id;

	// find accessed device
	{
		char *thisName;

		// search for device name
		for (id = 0; (thisName = gDeviceNames[id]) != NULL; id++) {
			if (!strcmp(name, thisName))
				break;
		}
		if (!thisName)
			return EINVAL;
	}

	vesa_info *info = gDeviceInfo[id];
	*_cookie = info;

	acquire_lock(&gLock);

	status_t status = B_OK;

	if (info->open_count++ == 0) {
		// this device has been opened for the first time, so
		// we allocate needed resources and initialize the structure
		if (status == B_OK)
			status = vesa_init(*info);
		if (status == B_OK)
			info->id = id;
	}

	release_lock(&gLock);
	return status;
}


static status_t
device_close(void *cookie)
{
	return B_OK;
}


static status_t
device_free(void *cookie)
{
	struct vesa_info *info = (vesa_info *)cookie;

	acquire_lock(&gLock);

	if (info->open_count-- == 1) {
		// release info structure
		vesa_uninit(*info);
	}

	release_lock(&gLock);
	return B_OK;
}


static status_t
device_ioctl(void *cookie, uint32 msg, void *buffer, size_t bufferLength)
{
	struct vesa_info *info = (vesa_info *)cookie;

	switch (msg) {
		case B_GET_ACCELERANT_SIGNATURE:
			user_strlcpy((char *)buffer, VESA_ACCELERANT_NAME, B_FILE_NAME_LENGTH);
			dprintf(DEVICE_NAME ": acc: %s\n", VESA_ACCELERANT_NAME);
			return B_OK;

		// needed to share data between kernel and accelerant		
		case VESA_GET_PRIVATE_DATA:
			return user_memcpy(buffer, &info->shared_area, sizeof(area_id));

		// needed for cloning
		case VESA_GET_DEVICE_NAME:
			if (user_strlcpy((char *)buffer, gDeviceNames[info->id], B_PATH_NAME_LENGTH) < B_OK)
				return B_BAD_ADDRESS;
			return B_OK;

		case VGA_SET_INDEXED_COLORS:
		{
			vga_set_indexed_colors_args args;
			if (user_memcpy(&args, buffer, sizeof(args)) < B_OK)
				return B_BAD_ADDRESS;

			return vga_set_indexed_colors(args.first, args.colors, args.count);
		}

		case VGA_PLANAR_BLIT:
		{
			vga_planar_blit_args args;
			if (user_memcpy(&args, buffer, sizeof(args)) < B_OK)
				return B_BAD_ADDRESS;

			return vga_planar_blit(info->shared_info, args.source,
				args.source_bytes_per_row, args.left, args.top,
				args.right, args.bottom);
		}

		default:
			TRACE((DEVICE_NAME ": ioctl() unknown message %ld (length = %lu)\n", msg, bufferLength));
	}

	return B_DEV_INVALID_IOCTL;
}


static status_t
device_read(void */*cookie*/, off_t /*pos*/, void */*buffer*/, size_t *_length)
{
	*_length = 0;	
	return B_NOT_ALLOWED;
}


static status_t
device_write(void */*cookie*/, off_t /*pos*/, const void */*buffer*/, size_t *_length)
{
	*_length = 0;	
	return B_NOT_ALLOWED;
}

