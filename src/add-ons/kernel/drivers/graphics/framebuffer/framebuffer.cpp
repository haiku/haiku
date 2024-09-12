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


static status_t
find_graphics_card(addr_t frameBuffer, addr_t& base, size_t& size)
{
	// TODO: when we port this over to the new driver API, this mechanism can be
	// used to find the right device_node
	pci_module_info* pci;
	if (get_module(B_PCI_MODULE_NAME, (module_info**)&pci) != B_OK)
		return B_ERROR;

	pci_info info;
	for (int32 index = 0; pci->get_nth_pci_info(index, &info) == B_OK; index++) {
		if (info.class_base != PCI_display)
			continue;

		// check PCI BARs
		for (uint32 i = 0; i < 6; i++) {
			phys_addr_t addr = info.u.h0.base_registers[i];
			uint64 barSize = info.u.h0.base_register_sizes[i];
			if (i < 5
				&& (info.u.h0.base_register_flags[i] & PCI_address_type) == PCI_address_type_64) {
				addr |= (uint64)info.u.h0.base_registers[i + 1] << 32;
				barSize |= (uint64)info.u.h0.base_register_sizes[i + 1] << 32;
				i++;
			}
			if (addr <= frameBuffer && addr + barSize > frameBuffer) {
				// found it!
				base = addr;
				size = barSize;
				dprintf(DEVICE_NAME " find_graphics_card: found base 0x%lx size %" B_PRIuSIZE "\n",
					base, size);

				put_module(B_PCI_MODULE_NAME);
				return B_OK;
			}
		}
	}

	dprintf(DEVICE_NAME " find_graphics_card: no entry found for 0x%lx\n", frameBuffer);
	put_module(B_PCI_MODULE_NAME);
	return B_ENTRY_NOT_FOUND;
}


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


/*!	Remaps the frame buffer if necessary; if we've already mapped the complete
	frame buffer, there is no need to map it again.
*/
static status_t
remap_frame_buffer(framebuffer_info& info, addr_t physicalBase, uint32 width,
	uint32 height, int8 depth, uint32 bytesPerRow, bool initializing)
{
	vesa_shared_info& sharedInfo = *info.shared_info;
	addr_t frameBuffer = info.frame_buffer;

	if (!info.complete_frame_buffer_mapped) {
		addr_t base = physicalBase;
		size_t size = bytesPerRow * height;
		// TODO: this logic looks suspicious and may need refactoring
		bool remap = !initializing || frameBuffer == 0;

		if (info.physical_frame_buffer_size != 0) {
			// we can map the complete frame buffer
			base = info.physical_frame_buffer;
			size = info.physical_frame_buffer_size;
			remap = true;
		}

		if (remap) {
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

			if (info.physical_frame_buffer_size != 0)
				info.complete_frame_buffer_mapped = true;
		}
	}

	if (info.complete_frame_buffer_mapped)
		frameBuffer += physicalBase - info.physical_frame_buffer;

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

	info.complete_frame_buffer_mapped = false;

	// Find out which PCI device we belong to, so that we know its frame buffer
	// size
	find_graphics_card(bufferInfo->physical_frame_buffer,
		info.physical_frame_buffer, info.physical_frame_buffer_size);

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
