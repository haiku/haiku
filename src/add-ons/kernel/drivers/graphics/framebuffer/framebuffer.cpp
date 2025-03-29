/*
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2016, Jessica Hamilton, jessica.l.hamilton@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include "framebuffer_private.h"
#include "vesa.h"

#include <string.h>

#include <drivers/bios.h>

#include <boot_item.h>
#include <frame_buffer_console.h>
#include <util/kernel_cpp.h>
#include <vm/vm.h>

#include "driver.h"
#include "utility.h"
#include "vesa_info.h"


static uint32
get_color_space_for_depth(uint32 depth)
{
	switch (depth) {
		case 1:
			return B_GRAY1;
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
remap_frame_buffer(framebuffer_info& info, addr_t physicalBase, uint32 width,
	uint32 height, int8 depth, uint32 bytesPerRow, bool initializing)
{
	vesa_shared_info& sharedInfo = *info.shared_info;
	addr_t frameBuffer = info.frame_buffer;

	addr_t base = physicalBase;
	size_t size = bytesPerRow * height;

	area_id area = map_physical_memory("framebuffer buffer", base,
		size, B_ANY_KERNEL_ADDRESS, B_READ_AREA | B_WRITE_AREA,
		(void**)&frameBuffer);
	if (area < 0)
		return area;

	if (initializing) {
		// We need to manually update the kernel's frame buffer address,
		// since this frame buffer remapping has not been issued by the
		// app_server (which would otherwise take care of this)
		frame_buffer_update(frameBuffer, width, height, depth,
			bytesPerRow);
	}

	delete_area(info.shared_info->frame_buffer_area);

	info.frame_buffer = frameBuffer;
	sharedInfo.frame_buffer_area = area;

	// Turn on write combining for the area
	vm_set_area_memory_type(area, base, B_WRITE_COMBINING_MEMORY);

	// Update shared frame buffer information
	sharedInfo.frame_buffer = (uint8*)frameBuffer;
	sharedInfo.physical_frame_buffer = (uint8*)physicalBase;
	sharedInfo.bytes_per_row = bytesPerRow;

	return B_OK;
}


//	#pragma mark -


status_t
framebuffer_init(framebuffer_info& info)
{
	frame_buffer_boot_info* bufferInfo
		= (frame_buffer_boot_info*)get_boot_item(FRAME_BUFFER_BOOT_INFO, NULL);
	if (bufferInfo == NULL)
		return B_ERROR;

	size_t sharedSize = (sizeof(vesa_shared_info) + 7) & ~7;

	info.shared_area = create_area("framebuffer shared info",
		(void**)&info.shared_info, B_ANY_KERNEL_ADDRESS,
		ROUND_TO_PAGE_SIZE(sharedSize), B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_CLONEABLE_AREA);
	if (info.shared_area < 0)
		return info.shared_area;

	vesa_shared_info& sharedInfo = *info.shared_info;

	memset(&sharedInfo, 0, sizeof(vesa_shared_info));

	sharedInfo.frame_buffer_area = bufferInfo->area;

	remap_frame_buffer(info, bufferInfo->physical_frame_buffer,
		bufferInfo->width, bufferInfo->height, bufferInfo->depth,
		bufferInfo->bytes_per_row, true);
		// Does not matter if this fails - the frame buffer was already mapped
		// before.

	sharedInfo.current_mode.virtual_width = bufferInfo->width;
	sharedInfo.current_mode.virtual_height = bufferInfo->height;
	sharedInfo.current_mode.space = get_color_space_for_depth(
		bufferInfo->depth);

	edid1_info* edidInfo = (edid1_info*)get_boot_item(VESA_EDID_BOOT_INFO,
		NULL);
	if (edidInfo != NULL) {
		sharedInfo.has_edid = true;
		memcpy(&sharedInfo.edid_info, edidInfo, sizeof(edid1_info));
	}

	dprintf(DEVICE_NAME ": framebuffer_init() completed successfully!\n");
	return B_OK;
}


void
framebuffer_uninit(framebuffer_info& info)
{
	dprintf(DEVICE_NAME": framebuffer_uninit()\n");

	delete_area(info.shared_info->frame_buffer_area);
	delete_area(info.shared_area);
}
