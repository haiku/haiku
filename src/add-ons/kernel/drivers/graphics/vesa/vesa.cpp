/*
 * Copyright 2005-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "vesa_private.h"
#include "vesa.h"

#include <string.h>

#include <boot_item.h>
#include <frame_buffer_console.h>
#include <util/kernel_cpp.h>
#include <arch/x86/vm86.h>

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
vbe_get_mode_info(struct vm86_state *vmState, uint16 mode,
	struct vbe_mode_info *modeInfo)
{
	struct vbe_mode_info *vbeModeInfo = (struct vbe_mode_info *)0x1000;

	memset(vbeModeInfo, 0, sizeof(vbe_mode_info));
	vmState->regs.eax = 0x4f01;
	vmState->regs.ecx = mode;
	vmState->regs.es  = 0x1000 >> 4;
	vmState->regs.edi = 0x0000;

	status_t status = vm86_do_int(vmState, 0x10);
	if (status != B_OK) {
		dprintf(DEVICE_NAME ": vbe_get_mode_info(%u): vm86 failed\n", mode);
		return status;
	}

	if ((vmState->regs.eax & 0xffff) != 0x4f) {
		dprintf(DEVICE_NAME ": vbe_get_mode_info(): BIOS returned 0x%04lx\n",
			vmState->regs.eax & 0xffff);
		return B_ENTRY_NOT_FOUND;
	}

	memcpy(modeInfo, vbeModeInfo, sizeof(struct vbe_mode_info));
	return B_OK;
}


static status_t
vbe_set_mode(struct vm86_state *vmState, uint16 mode)
{
	vmState->regs.eax = 0x4f02;
	vmState->regs.ebx = (mode & SET_MODE_MASK) | SET_MODE_LINEAR_BUFFER;

	status_t status = vm86_do_int(vmState, 0x10);
	if (status != B_OK) {
		dprintf(DEVICE_NAME ": vbe_set_mode(%u): vm86 failed\n", mode);
		return status;
	}

	if ((vmState->regs.eax & 0xffff) != 0x4f) {
		dprintf(DEVICE_NAME ": vbe_set_mode(): BIOS returned 0x%04lx\n",
			vmState->regs.eax & 0xffff);
		return B_ERROR;
	}

	return B_OK;
}


//	#pragma mark -


status_t
vesa_init(vesa_info &info)
{
	frame_buffer_boot_info *bufferInfo
		= (frame_buffer_boot_info *)get_boot_item(FRAME_BUFFER_BOOT_INFO, NULL);
	if (bufferInfo == NULL)
		return B_ERROR;

	size_t modesSize = 0;
	vesa_mode *modes = (vesa_mode *)get_boot_item(VESA_MODES_BOOT_INFO,
		&modesSize);
	info.modes = modes;

	size_t sharedSize = (sizeof(vesa_shared_info) + 7) & ~7;

	info.shared_area = create_area("vesa shared info",
		(void **)&info.shared_info, B_ANY_KERNEL_ADDRESS,
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
	sharedInfo.frame_buffer = (uint8 *)bufferInfo->frame_buffer;

	sharedInfo.current_mode.virtual_width = bufferInfo->width;
	sharedInfo.current_mode.virtual_height = bufferInfo->height;
	sharedInfo.current_mode.space = get_color_space_for_depth(
		bufferInfo->depth);
	sharedInfo.bytes_per_row = bufferInfo->bytes_per_row;

	// TODO: we might want to do this via vm86 instead
	edid1_info *edidInfo = (edid1_info *)get_boot_item(VESA_EDID_BOOT_INFO,
		NULL);
	if (edidInfo != NULL) {
		sharedInfo.has_edid = true;
		memcpy(&sharedInfo.edid_info, edidInfo, sizeof(edid1_info));
	}

	physical_entry mapping;
	get_memory_map((void *)sharedInfo.frame_buffer, B_PAGE_SIZE,
		&mapping, 1);
	sharedInfo.physical_frame_buffer = (uint8 *)mapping.address;

	dprintf(DEVICE_NAME ": vesa_init() completed successfully!\n");
	return B_OK;
}


void
vesa_uninit(vesa_info &info)
{
	dprintf(DEVICE_NAME": vesa_uninit()\n");

	delete_area(info.shared_info->frame_buffer_area);
	delete_area(info.shared_area);
}


status_t
vesa_set_display_mode(vesa_info &info, unsigned int mode)
{
	if (mode >= info.shared_info->vesa_mode_count)
		return B_ENTRY_NOT_FOUND;

	// Prepare vm86 mode environment
	struct vm86_state vmState;
	status_t status = vm86_prepare(&vmState, 0x2000);
	if (status != B_OK) {
		dprintf(DEVICE_NAME": vesa_set_display_mode(): vm86_prepare failed\n");
		return status;
	}

	area_id newFBArea;
	frame_buffer_boot_info *bufferInfo;
	struct vbe_mode_info modeInfo;

	// Get mode information
	status = vbe_get_mode_info(&vmState, info.modes[mode].mode, &modeInfo);
	if (status != B_OK) {
		dprintf(DEVICE_NAME": vesa_set_display_mode(): cannot get mode info\n");
		goto error;
	}

	// Set mode
	status = vbe_set_mode(&vmState, info.modes[mode].mode);
	if (status != B_OK) {
		dprintf(DEVICE_NAME": vesa_set_display_mode(): cannot set mode\n");
		goto error;
	}

	// Map new frame buffer
	void *frameBuffer;
	newFBArea = map_physical_memory("vesa_fb",
		(void *)modeInfo.physical_base,
		modeInfo.bytes_per_row * modeInfo.height, B_ANY_KERNEL_ADDRESS,
		B_READ_AREA | B_WRITE_AREA, &frameBuffer);
	if (newFBArea < B_OK) {
		status = (status_t)newFBArea;
		goto error;
	}
	delete_area(info.shared_info->frame_buffer_area);

	// Update shared frame buffer information
	info.shared_info->frame_buffer_area = newFBArea;
	info.shared_info->frame_buffer = (uint8 *)frameBuffer;
	info.shared_info->physical_frame_buffer = (uint8 *)modeInfo.physical_base;
	info.shared_info->bytes_per_row = modeInfo.bytes_per_row;
	info.shared_info->current_mode.virtual_width = modeInfo.width;
	info.shared_info->current_mode.virtual_height = modeInfo.height;
	info.shared_info->current_mode.space = get_color_space_for_depth(
		modeInfo.bits_per_pixel);

	// Update boot item as it's used in vesa_init()
	bufferInfo
		= (frame_buffer_boot_info *)get_boot_item(FRAME_BUFFER_BOOT_INFO, NULL);
	bufferInfo->area = newFBArea;
	bufferInfo->frame_buffer = (addr_t)frameBuffer;
	bufferInfo->width = modeInfo.width;
	bufferInfo->height = modeInfo.height;
	bufferInfo->depth = modeInfo.bits_per_pixel;
	bufferInfo->bytes_per_row = modeInfo.bytes_per_row;

error:
	vm86_cleanup(&vmState);
	return status;
}

