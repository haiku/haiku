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
		// This device hasn't been initialized yet, so we
		// allocate needed resources and initialize the structure
		info->init_status = radeon_hd_init(*info);
		if (info->init_status == B_OK) {
#ifdef DEBUG_COMMANDS
			add_debugger_command("radeonhd_reg", getset_register,
				"dumps or sets the specified radeon_hd register");
#endif
		}
	}

	if (info->init_status == B_OK) {
		info->open_count++;
		*_cookie = info;
	} else
		ERROR("%s: initialization failed!\n", __func__);

	mutex_unlock(&gLock);

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

#ifdef DEBUG_COMMANDS
        remove_debugger_command("radeonhd_reg", getset_register);
#endif
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
			TRACE("%s: accelerant: %s\n", __func__, RADEON_ACCELERANT_NAME);
			if (user_strlcpy((char*)buffer, RADEON_ACCELERANT_NAME,
					bufferLength) < B_OK)
				return B_BAD_ADDRESS;
			return B_OK;

		// needed to share data between kernel and accelerant
		case RADEON_GET_PRIVATE_DATA:
		{
			radeon_get_private_data data;
			if (user_memcpy(&data, buffer, sizeof(radeon_get_private_data)) < B_OK)
				return B_BAD_ADDRESS;

			if (data.magic == RADEON_PRIVATE_DATA_MAGIC) {
				data.shared_info_area = info->shared_area;
				return user_memcpy(buffer, &data,
					sizeof(radeon_get_private_data));
			}
			break;
		}

		// needed for cloning
		case RADEON_GET_DEVICE_NAME:
			if (user_strlcpy((char*)buffer, gDeviceNames[info->id],
					bufferLength) < B_OK)
				return B_BAD_ADDRESS;
			return B_OK;

		default:
			TRACE("%s: ioctl() unknown message %" B_PRIu32 " (length = %ld)\n",
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

