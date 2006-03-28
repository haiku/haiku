/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot_item.h>
#include <frame_buffer_console.h>

#include "vesa_info.h"
#include "driver.h"
#include "utility.h"

#include "util/kernel_cpp.h"

#include <string.h>


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
PhysicalMemoryMapper::Map(const char *name, void *physicalAddress, size_t numBytes,
	uint32 spec, uint32 protection, void **virtualAddress)
{
	fArea = map_physical_memory(name, physicalAddress, numBytes, spec, protection, virtualAddress);
	return fArea;
}


void 
PhysicalMemoryMapper::Keep()
{
	fArea = -1;
}


//	#pragma mark -


status_t
vesa_init(vesa_info &info)
{
	frame_buffer_boot_info *bufferInfo
		= (frame_buffer_boot_info *)get_boot_item(FRAME_BUFFER_BOOT_INFO);
	if (bufferInfo == NULL)
		return B_ERROR;

	info.shared_area = create_area("vesa shared info", (void **)&info.shared_info, 
			B_ANY_KERNEL_ADDRESS,
			ROUND_TO_PAGE_SIZE(sizeof(vesa_shared_info)),
			B_FULL_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_USER_CLONEABLE_AREA);
	if (info.shared_area < B_OK)
		return info.shared_area;

	memset((void *)info.shared_info, 0, sizeof(vesa_shared_info));

	info.shared_info->frame_buffer_area = bufferInfo->area;
	info.shared_info->frame_buffer = (uint8 *)bufferInfo->frame_buffer;

	info.shared_info->current_mode.virtual_width = bufferInfo->width;
	info.shared_info->current_mode.virtual_height = bufferInfo->height;
	switch (bufferInfo->depth) {
		case 4:
			// ???
			break;
		case 8:
			info.shared_info->current_mode.space = B_CMAP8;
			break;
		case 15:
			info.shared_info->current_mode.space = B_RGB15;
			break;
		case 16:
			info.shared_info->current_mode.space = B_RGB16;
			break;
		case 24:
			info.shared_info->current_mode.space = B_RGB24;
			break;
		case 32:
			info.shared_info->current_mode.space = B_RGB32;
			break;
	}
	info.shared_info->bytes_per_row = bufferInfo->bytes_per_row;

	physical_entry mapping;
	get_memory_map((void *)info.shared_info->frame_buffer, B_PAGE_SIZE, &mapping, 1);
	info.shared_info->physical_frame_buffer = (uint8 *)mapping.address;

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

