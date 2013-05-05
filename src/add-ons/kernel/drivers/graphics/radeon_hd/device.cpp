/*
 * Copyright 2006-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "driver.h"
#include "device.h"
#include "radeon_hd.h"
#include "utility.h"

#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <PCI.h>
#include <SupportDefs.h>
#include <graphic_driver.h>
#include <image.h>

#include <stdlib.h>
#include <string.h>


//#define DEBUG_COMMANDS

#define TRACE_DEVICE
#ifdef TRACE_DEVICE
#	define TRACE(x...) dprintf("radeon_hd: " x)
#else
#	define TRACE(x...) ;
#endif

#define ERROR(x...) dprintf("radeon_hd: " x)


/* device hooks prototypes */

static status_t device_open(const char* name, uint32 flags, void** _cookie);
static status_t device_close(void* data);
static status_t device_free(void* data);
static status_t device_ioctl(void* data, uint32 opcode,
	void* buffer, size_t length);
static status_t device_read(void* data, off_t offset,
	void* buffer, size_t* length);
static status_t device_write(void* data, off_t offset,
	const void* buffer, size_t* length);


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


#ifdef DEBUG_COMMANDS
static int
getset_register(int argc, char** argv)
{
	if (argc < 2 || argc > 3) {
		kprintf("usage: %s <register> [set-to-value]\n", argv[0]);
		return 0;
	}

	uint32 reg = parse_expression(argv[1]);
	uint32 value = 0;
	bool set = argc == 3;
	if (set)
		value = parse_expression(argv[2]);

	kprintf("radeon_hd register %#lx\n", reg);

	radeon_info &info = *gDeviceInfo[0];
	uint32 oldValue = read32(info.registers + reg);

	kprintf("  %svalue: %#lx (%lu)\n", set ? "old " : "", oldValue, oldValue);

	if (set) {
		write32(info.registers + reg, value);

		value = read32(info.registers + reg);
		kprintf("  new value: %#lx (%lu)\n", value, value);
	}

	return 0;
}
#endif	// DEBUG_COMMANDS


//	#pragma mark - Device Hooks


static status_t
device_open(const char* name, uint32 /*flags*/, void** _cookie)
{
	TRACE("%s: open(name = %s)\n", __func__, name);
	int32 id;

	// find accessed device
	{
		char* thisName;

		// search for device name
		for (id = 0; (thisName = gDeviceNames[id]) != NULL; id++) {
			if (!strcmp(name, thisName))
				break;
		}
		if (!thisName)
			return B_BAD_VALUE;
	}

	radeon_info* info = gDeviceInfo[id];

	mutex_lock(&gLock);

	if (info->open_count == 0) {
		// this device has been opened for the first time, so
		// we allocate needed resources and initialize the structure
		info->init_status = radeon_hd_init(*info);
		if (info->init_status == B_OK) {
			info->open_count++;
		}
	}

	mutex_unlock(&gLock);

	if (info->init_status == B_OK)
		*_cookie = info;

	return info->init_status;
}


static status_t
device_close(void* /*data*/)
{
	TRACE("%s: close\n", __func__);
	return B_OK;
}


static status_t
device_free(void* data)
{
	struct radeon_info* info = (radeon_info*)data;

	mutex_lock(&gLock);

	if (info->open_count-- == 1) {
		// release info structure
		info->init_status = B_NO_INIT;
		radeon_hd_uninit(*info);
	}
	mutex_unlock(&gLock);
	return B_OK;
}


static status_t
device_ioctl(void* data, uint32 op, void* buffer, size_t bufferLength)
{
	struct radeon_info* info = (radeon_info*)data;

	switch (op) {
		case B_GET_ACCELERANT_SIGNATURE:
			strcpy((char*)buffer, RADEON_ACCELERANT_NAME);
			TRACE("%s: accelerant: %s\n", __func__, RADEON_ACCELERANT_NAME);
			return B_OK;

		// needed to share data between kernel and accelerant
		case RADEON_GET_PRIVATE_DATA:
		{
			radeon_get_private_data* data = (radeon_get_private_data*)buffer;

			if (data->magic == RADEON_PRIVATE_DATA_MAGIC) {
				data->shared_info_area = info->shared_area;
				return B_OK;
			}
			break;
		}

		// needed for cloning
		case RADEON_GET_DEVICE_NAME:
#ifdef __HAIKU__
			if (user_strlcpy((char*)buffer, gDeviceNames[info->id],
					B_PATH_NAME_LENGTH) < B_OK)
				return B_BAD_ADDRESS;
#else
			strncpy((char*)buffer, gDeviceNames[info->id], B_PATH_NAME_LENGTH);
			((char*)buffer)[B_PATH_NAME_LENGTH - 1] = '\0';
#endif
			return B_OK;

		default:
			TRACE("%s: ioctl() unknown message %ld (length = %ld)\n",
				__func__, op, bufferLength);
			break;
	}

	return B_DEV_INVALID_IOCTL;
}


static status_t
device_read(void* /*data*/, off_t /*pos*/,
	void* /*buffer*/, size_t *_length)
{
	*_length = 0;
	return B_NOT_ALLOWED;
}


static status_t
device_write(void* /*data*/, off_t /*pos*/,
	const void* /*buffer*/, size_t* _length)
{
	*_length = 0;
	return B_NOT_ALLOWED;
}

