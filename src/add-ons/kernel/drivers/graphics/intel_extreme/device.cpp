/*
 * Copyright 2006-2018, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "driver.h"
#include "device.h"
#include "intel_extreme.h"
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


#define DEBUG_COMMANDS

#define TRACE_DEVICE
#ifdef TRACE_DEVICE
#	define TRACE(x...) dprintf("intel_extreme: " x)
#else
#	define TRACE(x) ;
#endif

#define ERROR(x...) dprintf("intel_extreme: " x)
#define CALLED(x...) TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


/* device hooks prototypes */

static status_t device_open(const char* name, uint32 flags, void** _cookie);
static status_t device_close(void* data);
static status_t device_free(void* data);
static status_t device_ioctl(void* data, uint32 opcode, void* buffer,
	size_t length);
static status_t device_read(void* data, off_t offset, void* buffer,
	size_t* length);
static status_t device_write(void* data, off_t offset, const void* buffer,
	size_t* length);


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

	kprintf("intel_extreme register %#" B_PRIx32 "\n", reg);

	intel_info &info = *gDeviceInfo[0];
	uint32 oldValue = read32(info, reg);

	kprintf("  %svalue: %#" B_PRIx32 " (%" B_PRIu32 ")\n", set ? "old " : "",
		oldValue, oldValue);

	if (set) {
		write32(info, reg, value);

		value = read32(info, reg);
		kprintf("  new value: %#" B_PRIx32 " (%" B_PRIu32 ")\n", value, value);
	}

	return 0;
}


static int
dump_pipe_info(int argc, char** argv)
{
	int pipeOffset = 0;

	if (argc > 2) {
		kprintf("usage: %s [pipe index]\n", argv[0]);
		return 0;
	}

	if (argc > 1) {
		uint32 pipe = parse_expression(argv[1]);
		if (pipe != 0)
			pipeOffset = INTEL_DISPLAY_OFFSET; // Use pipe B if requested
	}

	intel_info &info = *gDeviceInfo[0];
	uint32 value;

	kprintf("intel_extreme pipe configuration:\n");

	value = read32(info, INTEL_DISPLAY_A_HTOTAL + pipeOffset);
	kprintf("  HTOTAL start %" B_PRIu32 " end %" B_PRIu32 "\n",
		(value & 0xFFFF) + 1, (value >> 16) + 1);
	value = read32(info, INTEL_DISPLAY_A_HBLANK + pipeOffset);
	kprintf("  HBLANK start %" B_PRIu32 " end %" B_PRIu32 "\n",
		(value & 0xFFFF) + 1, (value >> 16) + 1);
	value = read32(info, INTEL_DISPLAY_A_HSYNC + pipeOffset);
	kprintf("  HSYNC start %" B_PRIu32 " end %" B_PRIu32 "\n",
		(value & 0xFFFF) + 1, (value >> 16) + 1);
	value = read32(info, INTEL_DISPLAY_A_VTOTAL + pipeOffset);
	kprintf("  VTOTAL start %" B_PRIu32 " end %" B_PRIu32 "\n",
		(value & 0xFFFF) + 1, (value >> 16) + 1);
	value = read32(info, INTEL_DISPLAY_A_VBLANK + pipeOffset);
	kprintf("  VBLANK start %" B_PRIu32 " end %" B_PRIu32 "\n",
		(value & 0xFFFF) + 1, (value >> 16) + 1);
	value = read32(info, INTEL_DISPLAY_A_VSYNC + pipeOffset);
	kprintf("  VSYNC start %" B_PRIu32 " end %" B_PRIu32 "\n",
		(value & 0xFFFF) + 1, (value >> 16) + 1);
	value = read32(info, INTEL_DISPLAY_A_PIPE_SIZE + pipeOffset);
	kprintf("  SIZE %" B_PRIu32 "x%" B_PRIu32 "\n",
		(value & 0xFFFF) + 1, (value >> 16) + 1);

	if (info.pch_info != INTEL_PCH_NONE) {
		kprintf("intel_extreme transcoder configuration:\n");

		value = read32(info, INTEL_TRANSCODER_A_HTOTAL + pipeOffset);
		kprintf("  HTOTAL start %" B_PRIu32 " end %" B_PRIu32 "\n",
			(value & 0xFFFF) + 1, (value >> 16) + 1);
		value = read32(info, INTEL_TRANSCODER_A_HBLANK + pipeOffset);
		kprintf("  HBLANK start %" B_PRIu32 " end %" B_PRIu32 "\n",
			(value & 0xFFFF) + 1, (value >> 16) + 1);
		value = read32(info, INTEL_TRANSCODER_A_HSYNC + pipeOffset);
		kprintf("  HSYNC start %" B_PRIu32 " end %" B_PRIu32 "\n",
			(value & 0xFFFF) + 1, (value >> 16) + 1);
		value = read32(info, INTEL_TRANSCODER_A_VTOTAL + pipeOffset);
		kprintf("  VTOTAL start %" B_PRIu32 " end %" B_PRIu32 "\n",
			(value & 0xFFFF) + 1, (value >> 16) + 1);
		value = read32(info, INTEL_TRANSCODER_A_VBLANK + pipeOffset);
		kprintf("  VBLANK start %" B_PRIu32 " end %" B_PRIu32 "\n",
			(value & 0xFFFF) + 1, (value >> 16) + 1);
		value = read32(info, INTEL_TRANSCODER_A_VSYNC + pipeOffset);
		kprintf("  VSYNC start %" B_PRIu32 " end %" B_PRIu32 "\n",
			(value & 0xFFFF) + 1, (value >> 16) + 1);
		value = read32(info, INTEL_TRANSCODER_A_IMAGE_SIZE + pipeOffset);
		kprintf("  SIZE %" B_PRIu32 "x%" B_PRIu32 "\n",
			(value & 0xFFFF) + 1, (value >> 16) + 1);
	}

	kprintf("intel_extreme display plane configuration:\n");

	value = read32(info, INTEL_DISPLAY_A_CONTROL + pipeOffset);
	kprintf("  CONTROL: %" B_PRIx32 "\n", value);
	value = read32(info, INTEL_DISPLAY_A_BASE + pipeOffset);
	kprintf("  BASE: %" B_PRIx32 "\n", value);
	value = read32(info, INTEL_DISPLAY_A_BYTES_PER_ROW + pipeOffset);
	kprintf("  BYTES_PER_ROW: %" B_PRIx32 "\n", value);
	value = read32(info, INTEL_DISPLAY_A_SURFACE + pipeOffset);
	kprintf("  SURFACE: %" B_PRIx32 "\n", value);

	return 0;
}

#endif	// DEBUG_COMMANDS


//	#pragma mark - Device Hooks


static status_t
device_open(const char* name, uint32 /*flags*/, void** _cookie)
{
	CALLED();
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

	intel_info* info = gDeviceInfo[id];

	mutex_lock(&gLock);

	if (info->open_count == 0) {
		// This device hasn't been initialized yet, so we
		// allocate needed resources and initialize the structure
		info->init_status = intel_extreme_init(*info);
		if (info->init_status == B_OK) {
#ifdef DEBUG_COMMANDS
			add_debugger_command("ie_reg", getset_register,
				"dumps or sets the specified intel_extreme register");
			add_debugger_command("ie_pipe", dump_pipe_info,
				"show pipe configuration information");
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
	CALLED();
	return B_OK;
}


static status_t
device_free(void* data)
{
	struct intel_info* info = (intel_info*)data;

	mutex_lock(&gLock);

	if (info->open_count-- == 1) {
		// release info structure
		info->init_status = B_NO_INIT;
		intel_extreme_uninit(*info);

#ifdef DEBUG_COMMANDS
		remove_debugger_command("ie_reg", getset_register);
		remove_debugger_command("ie_pipe", dump_pipe_info);
#endif
	}

	mutex_unlock(&gLock);
	return B_OK;
}


static status_t
device_ioctl(void* data, uint32 op, void* buffer, size_t bufferLength)
{
	struct intel_info* info = (intel_info*)data;

	switch (op) {
		case B_GET_ACCELERANT_SIGNATURE:
			TRACE("accelerant: %s\n", INTEL_ACCELERANT_NAME);
			if (user_strlcpy((char*)buffer, INTEL_ACCELERANT_NAME,
					bufferLength) < B_OK)
				return B_BAD_ADDRESS;
			return B_OK;

		// needed to share data between kernel and accelerant
		case INTEL_GET_PRIVATE_DATA:
		{
			intel_get_private_data data;
			if (user_memcpy(&data, buffer, sizeof(intel_get_private_data)) < B_OK)
				return B_BAD_ADDRESS;

			if (data.magic == INTEL_PRIVATE_DATA_MAGIC) {
				data.shared_info_area = info->shared_area;
				return user_memcpy(buffer, &data,
					sizeof(intel_get_private_data));
			}
			break;
		}

		// needed for cloning
		case INTEL_GET_DEVICE_NAME:
			if (user_strlcpy((char* )buffer, gDeviceNames[info->id],
					bufferLength) < B_OK)
				return B_BAD_ADDRESS;
			return B_OK;

		// graphics mem manager
		case INTEL_ALLOCATE_GRAPHICS_MEMORY:
		{
			intel_allocate_graphics_memory allocMemory;
			if (user_memcpy(&allocMemory, buffer,
					sizeof(intel_allocate_graphics_memory)) < B_OK)
				return B_BAD_ADDRESS;

			if (allocMemory.magic != INTEL_PRIVATE_DATA_MAGIC)
				return B_BAD_VALUE;

			status_t status = intel_allocate_memory(*info, allocMemory.size,
				allocMemory.alignment, allocMemory.flags,
				&allocMemory.buffer_base);
			if (status == B_OK) {
				// copy result
				if (user_memcpy(buffer, &allocMemory,
						sizeof(intel_allocate_graphics_memory)) < B_OK)
					return B_BAD_ADDRESS;
			}
			return status;
		}

		case INTEL_FREE_GRAPHICS_MEMORY:
		{
			intel_free_graphics_memory freeMemory;
			if (user_memcpy(&freeMemory, buffer,
					sizeof(intel_free_graphics_memory)) < B_OK)
				return B_BAD_ADDRESS;

			if (freeMemory.magic == INTEL_PRIVATE_DATA_MAGIC)
				return intel_free_memory(*info, freeMemory.buffer_base);
			break;
		}

		default:
			ERROR("ioctl() unknown message %" B_PRIu32 " (length = %"
				B_PRIuSIZE ")\n", op, bufferLength);
			break;
	}

	return B_DEV_INVALID_IOCTL;
}


static status_t
device_read(void* /*data*/, off_t /*pos*/, void* /*buffer*/, size_t* _length)
{
	*_length = 0;
	return B_NOT_ALLOWED;
}


static status_t
device_write(void* /*data*/, off_t /*pos*/, const void* /*buffer*/,
	size_t* _length)
{
	*_length = 0;
	return B_NOT_ALLOWED;
}

