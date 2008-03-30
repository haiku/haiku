/*
 * Copyright 2005-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "vesa_private.h"

#include <string.h>

#include <boot_item.h>
#include <frame_buffer_console.h>
#include <util/kernel_cpp.h>

#include "driver.h"
#include "utility.h"
#include "vesa_info.h"


class PhysicalMemoryMapper {
	public:
		PhysicalMemoryMapper();
		~PhysicalMemoryMapper();

		area_id Map(const char *name, void *physicalAddress, size_t numBytes,
			uint32 spec, uint32 protection, void **virtualAddress);
		status_t InitCheck() { return fArea < B_OK ? (status_t)fArea : B_OK; }
		void Keep();

	private:
		area_id	fArea;
};


PhysicalMemoryMapper::PhysicalMemoryMapper()
	:
	fArea(-1)
{
}


PhysicalMemoryMapper::~PhysicalMemoryMapper()
{
	if (fArea >= B_OK)
		delete_area(fArea);
}


area_id 
PhysicalMemoryMapper::Map(const char *name, void *physicalAddress,
	size_t numBytes, uint32 spec, uint32 protection, void **virtualAddress)
{
	fArea = map_physical_memory(name, physicalAddress, numBytes, spec,
		protection, virtualAddress);
	return fArea;
}


void 
PhysicalMemoryMapper::Keep()
{
	fArea = -1;
}


//	#pragma mark -


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

	size_t sharedSize = (sizeof(vesa_shared_info) + 7) & ~7;

	info.shared_area = create_area("vesa shared info",
		(void **)&info.shared_info, B_ANY_KERNEL_ADDRESS,
		ROUND_TO_PAGE_SIZE(sharedSize + modesSize), B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_USER_CLONEABLE_AREA);
	if (info.shared_area < B_OK)
		return info.shared_area;

	vesa_shared_info &sharedInfo = *info.shared_info;

	memset(/*(void *)*/&sharedInfo, 0, sizeof(vesa_shared_info));

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

