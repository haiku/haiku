/*
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "vesa_private.h"
#include "vesa.h"

#include <string.h>

#include <boot_item.h>
#include <frame_buffer_console.h>
#include <util/kernel_cpp.h>
#include <arch/x86/vm86.h>
#include <vm.h>

#include "driver.h"
#include "utility.h"
#include "vesa_info.h"


static uint32
get_color_space_for_depth(uint32 depth)
{
	switch (depth) {
		case 4:
			return B_GRAY8;
				// the app_server is smart enough to translate this to VGA mode
		case 8:
			return B_CMAP8;
		case 15:
			return B_RGB15;
		case 16:
			return B_RGB16;
		case 24:
			return B_RGB24;
		case 32:
			return B_RGB32;
	}

	return 0;
}


static status_t
vbe_get_mode_info(struct vm86_state& vmState, uint16 mode,
	struct vbe_mode_info* modeInfo)
{
	struct vbe_mode_info* vbeModeInfo = (struct vbe_mode_info*)0x1000;

	memset(vbeModeInfo, 0, sizeof(vbe_mode_info));
	vmState.regs.eax = 0x4f01;
	vmState.regs.ecx = mode;
	vmState.regs.es  = 0x1000 >> 4;
	vmState.regs.edi = 0x0000;

	status_t status = vm86_do_int(&vmState, 0x10);
	if (status != B_OK) {
		dprintf(DEVICE_NAME ": vbe_get_mode_info(%u): vm86 failed\n", mode);
		return status;
	}

	if ((vmState.regs.eax & 0xffff) != 0x4f) {
		dprintf(DEVICE_NAME ": vbe_get_mode_info(): BIOS returned 0x%04lx\n",
			vmState.regs.eax & 0xffff);
		return B_ENTRY_NOT_FOUND;
	}

	memcpy(modeInfo, vbeModeInfo, sizeof(struct vbe_mode_info));
	return B_OK;
}


static status_t
vbe_set_mode(struct vm86_state& vmState, uint16 mode)
{
	vmState.regs.eax = 0x4f02;
	vmState.regs.ebx = (mode & SET_MODE_MASK) | SET_MODE_LINEAR_BUFFER;

	status_t status = vm86_do_int(&vmState, 0x10);
	if (status != B_OK) {
		dprintf(DEVICE_NAME ": vbe_set_mode(%u): vm86 failed\n", mode);
		return status;
	}

	if ((vmState.regs.eax & 0xffff) != 0x4f) {
		dprintf(DEVICE_NAME ": vbe_set_mode(): BIOS returned 0x%04lx\n",
			vmState.regs.eax & 0xffff);
		return B_ERROR;
	}

	return B_OK;
}


static uint32
vbe_to_system_dpms(uint8 vbeMode)
{
	uint32 mode = 0;
	if ((vbeMode & (DPMS_OFF | DPMS_REDUCED_ON)) != 0)
		mode |= B_DPMS_OFF;
	if ((vbeMode & DPMS_STANDBY) != 0)
		mode |= B_DPMS_STAND_BY;
	if ((vbeMode & DPMS_SUSPEND) != 0)
		mode |= B_DPMS_SUSPEND;

	return mode;
}


static status_t
vbe_get_dpms_capabilities(uint32& vbeMode, uint32& mode)
{
	// we always return a valid mode
	vbeMode = 0;
	mode = B_DPMS_ON;

	// Prepare vm86 mode environment
	struct vm86_state vmState;
	status_t status = vm86_prepare(&vmState, 0x20000);
	if (status != B_OK) {
		dprintf(DEVICE_NAME": vbe_get_dpms_capabilities(): vm86_prepare "
			"failed: %s\n", strerror(status));
		return status;
	}

	vmState.regs.eax = 0x4f10;
	vmState.regs.ebx = 0;
	vmState.regs.esi = 0;
	vmState.regs.edi = 0;

	status = vm86_do_int(&vmState, 0x10);
	if (status != B_OK) {
		dprintf(DEVICE_NAME ": vbe_get_dpms_capabilities(): vm86 failed\n");
		goto out;
	}

	if ((vmState.regs.eax & 0xffff) != 0x4f) {
		dprintf(DEVICE_NAME ": vbe_get_dpms_capabilities(): BIOS returned "
			"0x%04lx\n", vmState.regs.eax & 0xffff);
		status = B_ERROR;
		goto out;
	}

	vbeMode = vmState.regs.ebx >> 8;
	mode = vbe_to_system_dpms(vbeMode);

out:
	vm86_cleanup(&vmState);
	return status;
}


//	#pragma mark -


status_t
vesa_init(vesa_info& info)
{
	frame_buffer_boot_info* bufferInfo
		= (frame_buffer_boot_info*)get_boot_item(FRAME_BUFFER_BOOT_INFO, NULL);
	if (bufferInfo == NULL)
		return B_ERROR;

	size_t modesSize = 0;
	vesa_mode* modes = (vesa_mode*)get_boot_item(VESA_MODES_BOOT_INFO,
		&modesSize);
	info.modes = modes;

	size_t sharedSize = (sizeof(vesa_shared_info) + 7) & ~7;

	info.shared_area = create_area("vesa shared info",
		(void**)&info.shared_info, B_ANY_KERNEL_ADDRESS,
		ROUND_TO_PAGE_SIZE(sharedSize + modesSize), B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_USER_CLONEABLE_AREA);
	if (info.shared_area < B_OK)
		return info.shared_area;

	vesa_shared_info &sharedInfo = *info.shared_info;

	memset(&sharedInfo, 0, sizeof(vesa_shared_info));

	if (modes != NULL) {
		sharedInfo.vesa_mode_offset = sharedSize;
		sharedInfo.vesa_mode_count = modesSize / sizeof(vesa_mode);

		memcpy((uint8*)&sharedInfo + sharedSize, modes, modesSize);
	}

	sharedInfo.frame_buffer_area = bufferInfo->area;
	sharedInfo.frame_buffer = (uint8*)bufferInfo->frame_buffer;

	sharedInfo.current_mode.virtual_width = bufferInfo->width;
	sharedInfo.current_mode.virtual_height = bufferInfo->height;
	sharedInfo.current_mode.space = get_color_space_for_depth(
		bufferInfo->depth);
	sharedInfo.bytes_per_row = bufferInfo->bytes_per_row;

	// TODO: we might want to do this via vm86 instead
	edid1_info* edidInfo = (edid1_info*)get_boot_item(VESA_EDID_BOOT_INFO,
		NULL);
	if (edidInfo != NULL) {
		sharedInfo.has_edid = true;
		memcpy(&sharedInfo.edid_info, edidInfo, sizeof(edid1_info));
	}

	vbe_get_dpms_capabilities(info.vbe_dpms_capabilities,
		sharedInfo.dpms_capabilities);

	physical_entry mapping;
	get_memory_map((void*)sharedInfo.frame_buffer, B_PAGE_SIZE, &mapping, 1);
	sharedInfo.physical_frame_buffer = (uint8*)mapping.address;

	dprintf(DEVICE_NAME ": vesa_init() completed successfully!\n");
	return B_OK;
}


void
vesa_uninit(vesa_info& info)
{
	dprintf(DEVICE_NAME": vesa_uninit()\n");

	delete_area(info.shared_info->frame_buffer_area);
	delete_area(info.shared_area);
}


status_t
vesa_set_display_mode(vesa_info& info, uint32 mode)
{
	if (mode >= info.shared_info->vesa_mode_count)
		return B_ENTRY_NOT_FOUND;

	// Prepare vm86 mode environment
	struct vm86_state vmState;
	status_t status = vm86_prepare(&vmState, 0x20000);
	if (status != B_OK) {
		dprintf(DEVICE_NAME": vesa_set_display_mode(): vm86_prepare failed\n");
		return status;
	}

	area_id frameBufferArea;
	frame_buffer_boot_info* bufferInfo;
	struct vbe_mode_info modeInfo;

	// Get mode information
	status = vbe_get_mode_info(vmState, info.modes[mode].mode, &modeInfo);
	if (status != B_OK) {
		dprintf(DEVICE_NAME": vesa_set_display_mode(): cannot get mode info\n");
		goto out;
	}

	// Set mode
	status = vbe_set_mode(vmState, info.modes[mode].mode);
	if (status != B_OK) {
		dprintf(DEVICE_NAME": vesa_set_display_mode(): cannot set mode\n");
		goto out;
	}

	// Map new frame buffer
	void* frameBuffer;
	frameBufferArea = map_physical_memory("vesa_fb",
		(void*)modeInfo.physical_base, modeInfo.bytes_per_row * modeInfo.height,
		B_ANY_KERNEL_ADDRESS, B_READ_AREA | B_WRITE_AREA, &frameBuffer);
	if (frameBufferArea < B_OK) {
		status = (status_t)frameBufferArea;
		goto out;
	}
	delete_area(info.shared_info->frame_buffer_area);

	// Turn on write combining for the area
	vm_set_area_memory_type(frameBufferArea, modeInfo.physical_base, B_MTR_WC);

	// Update shared frame buffer information
	info.shared_info->frame_buffer_area = frameBufferArea;
	info.shared_info->frame_buffer = (uint8*)frameBuffer;
	info.shared_info->physical_frame_buffer = (uint8*)modeInfo.physical_base;
	info.shared_info->bytes_per_row = modeInfo.bytes_per_row;
	info.shared_info->current_mode.virtual_width = modeInfo.width;
	info.shared_info->current_mode.virtual_height = modeInfo.height;
	info.shared_info->current_mode.space = get_color_space_for_depth(
		modeInfo.bits_per_pixel);

	// Update boot item as it's used in vesa_init()
	bufferInfo
		= (frame_buffer_boot_info*)get_boot_item(FRAME_BUFFER_BOOT_INFO, NULL);
	bufferInfo->area = frameBufferArea;
	bufferInfo->frame_buffer = (addr_t)frameBuffer;
	bufferInfo->width = modeInfo.width;
	bufferInfo->height = modeInfo.height;
	bufferInfo->depth = modeInfo.bits_per_pixel;
	bufferInfo->bytes_per_row = modeInfo.bytes_per_row;

out:
	vm86_cleanup(&vmState);
	return status;
}


status_t
vesa_get_dpms_mode(vesa_info& info, uint32& mode)
{
	mode = B_DPMS_ON;
		// we always return a valid mode

	// Prepare vm86 mode environment
	struct vm86_state vmState;
	status_t status = vm86_prepare(&vmState, 0x20000);
	if (status != B_OK) {
		dprintf(DEVICE_NAME": vesa_get_dpms_mode(): vm86_prepare failed: %s\n",
			strerror(status));
		return status;
	}

	vmState.regs.eax = 0x4f10;
	vmState.regs.ebx = 2;
	vmState.regs.esi = 0;
	vmState.regs.edi = 0;

	status = vm86_do_int(&vmState, 0x10);
	if (status != B_OK) {
		dprintf(DEVICE_NAME ": vesa_get_dpms_mode(): vm86 failed: %s\n",
			strerror(status));
		goto out;
	}

	if ((vmState.regs.eax & 0xffff) != 0x4f) {
		dprintf(DEVICE_NAME ": vesa_get_dpms_mode(): BIOS returned 0x%04lx\n",
			vmState.regs.eax & 0xffff);
		status = B_ERROR;
		goto out;
	}

	mode = vbe_to_system_dpms(vmState.regs.ebx >> 8);

out:
	vm86_cleanup(&vmState);
	return status;
}


status_t
vesa_set_dpms_mode(vesa_info& info, uint32 mode)
{
	// Only let supported modes through
	mode &= info.shared_info->dpms_capabilities;

	uint8 vbeMode = 0;
	if ((mode & B_DPMS_OFF) != 0)
		vbeMode |= DPMS_OFF | DPMS_REDUCED_ON;
	if ((mode & B_DPMS_STAND_BY) != 0)
		vbeMode |= DPMS_STANDBY;
	if ((mode & B_DPMS_SUSPEND) != 0)
		vbeMode |= DPMS_SUSPEND;

	vbeMode &= info.vbe_dpms_capabilities;

	// Prepare vm86 mode environment
	struct vm86_state vmState;
	status_t status = vm86_prepare(&vmState, 0x20000);
	if (status != B_OK) {
		dprintf(DEVICE_NAME": vesa_set_dpms_mode(): vm86_prepare failed: %s\n",
			strerror(status));
		return status;
	}

	vmState.regs.eax = 0x4f10;
	vmState.regs.ebx = (vbeMode << 8) | 1;
	vmState.regs.esi = 0;
	vmState.regs.edi = 0;

	status = vm86_do_int(&vmState, 0x10);
	if (status != B_OK) {
		dprintf(DEVICE_NAME ": vesa_set_dpms_mode(): vm86 failed: %s\n",
			strerror(status));
		goto out;
	}

	if ((vmState.regs.eax & 0xffff) != 0x4f) {
		dprintf(DEVICE_NAME ": vesa_set_dpms_mode(): BIOS returned 0x%04lx\n",
			vmState.regs.eax & 0xffff);
		status = B_ERROR;
		goto out;
	}

out:
	vm86_cleanup(&vmState);
	return status;
}
