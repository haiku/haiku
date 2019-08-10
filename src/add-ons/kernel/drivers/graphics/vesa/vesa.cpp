/*
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "vesa_private.h"
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


static bios_module_info* sBIOSModule;


/*!	Loads the BIOS module and sets up a state for it. The BIOS module is only
	loaded when we need it, as it is quite a large module.
*/
static status_t
vbe_call_prepare(bios_state** state)
{
	status_t status;

	status = get_module(B_BIOS_MODULE_NAME, (module_info**)&sBIOSModule);
	if (status != B_OK) {
		dprintf(DEVICE_NAME ": failed to get BIOS module: %s\n",
			strerror(status));
		return status;
	}

	status = sBIOSModule->prepare(state);
	if (status != B_OK) {
		dprintf(DEVICE_NAME ": failed to prepare BIOS state: %s\n",
			strerror(status));
		put_module(B_BIOS_MODULE_NAME);
	}

	return status;
}


static void
vbe_call_finish(bios_state* state)
{
	sBIOSModule->finish(state);
	put_module(B_BIOS_MODULE_NAME);
}


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
			if (info.u.h0.base_registers[i] <= frameBuffer
				&& info.u.h0.base_registers[i] + info.u.h0.base_register_sizes[i]
					> frameBuffer) {
				// found it!
				base = info.u.h0.base_registers[i];
				size = info.u.h0.base_register_sizes[i];

				put_module(B_PCI_MODULE_NAME);
				return B_OK;
			}
		}
	}

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


static status_t
vbe_get_mode_info(bios_state* state, uint16 mode, struct vbe_mode_info* modeInfo)
{
	void* vbeModeInfo = sBIOSModule->allocate_mem(state,
		sizeof(struct vbe_mode_info));
	if (vbeModeInfo == NULL)
		return B_NO_MEMORY;
	memset(vbeModeInfo, 0, sizeof(vbe_mode_info));

	uint32 physicalAddress = sBIOSModule->physical_address(state, vbeModeInfo);
	bios_regs regs = {};
	regs.eax = 0x4f01;
	regs.ecx = mode;
	regs.es  = physicalAddress >> 4;
	regs.edi = physicalAddress - (regs.es << 4);

	status_t status = sBIOSModule->interrupt(state, 0x10, &regs);
	if (status != B_OK) {
		dprintf(DEVICE_NAME ": vbe_get_mode_info(%u): BIOS failed: %s\n", mode,
			strerror(status));
		return status;
	}

	if ((regs.eax & 0xffff) != 0x4f) {
		dprintf(DEVICE_NAME ": vbe_get_mode_info(%u): BIOS returned "
			"0x%04" B_PRIx32 "\n", mode, regs.eax & 0xffff);
		return B_ENTRY_NOT_FOUND;
	}

	memcpy(modeInfo, vbeModeInfo, sizeof(struct vbe_mode_info));
	return B_OK;
}


static status_t
vbe_set_mode(bios_state* state, uint16 mode)
{
	bios_regs regs = {};
	regs.eax = 0x4f02;
	regs.ebx = (mode & SET_MODE_MASK) | SET_MODE_LINEAR_BUFFER;

	status_t status = sBIOSModule->interrupt(state, 0x10, &regs);
	if (status != B_OK) {
		dprintf(DEVICE_NAME ": vbe_set_mode(%u): BIOS failed: %s\n", mode,
			strerror(status));
		return status;
	}

	if ((regs.eax & 0xffff) != 0x4f) {
		dprintf(DEVICE_NAME ": vbe_set_mode(%u): BIOS returned 0x%04" B_PRIx32
			"\n", mode, regs.eax & 0xffff);
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

	// Prepare BIOS environment
	bios_state* state;
	status_t status = vbe_call_prepare(&state);
	if (status != B_OK)
		return status;

	bios_regs regs = {};
	regs.eax = 0x4f10;
	regs.ebx = 0;
	regs.esi = 0;
	regs.edi = 0;

	status = sBIOSModule->interrupt(state, 0x10, &regs);
	if (status != B_OK) {
		dprintf(DEVICE_NAME ": vbe_get_dpms_capabilities(): BIOS failed: %s\n",
			strerror(status));
		goto out;
	}

	if ((regs.eax & 0xffff) != 0x4f) {
		dprintf(DEVICE_NAME ": vbe_get_dpms_capabilities(): BIOS returned "
			"0x%04" B_PRIx32 "\n", regs.eax & 0xffff);
		status = B_ERROR;
		goto out;
	}

	vbeMode = regs.ebx >> 8;
	mode = vbe_to_system_dpms(vbeMode);

out:
	vbe_call_finish(state);
	return status;
}


static status_t
vbe_set_bits_per_gun(bios_state* state, vesa_info& info, uint8 bits)
{
	info.bits_per_gun = 6;

	bios_regs regs = {};
	regs.eax = 0x4f08;
	regs.ebx = (bits << 8) | 1;

	status_t status = sBIOSModule->interrupt(state, 0x10, &regs);
	if (status != B_OK) {
		dprintf(DEVICE_NAME ": vbe_set_bits_per_gun(): BIOS failed: %s\n",
			strerror(status));
		return status;
	}

	if ((regs.eax & 0xffff) != 0x4f) {
		dprintf(DEVICE_NAME ": vbe_set_bits_per_gun(): BIOS returned "
			"0x%04" B_PRIx32 "\n", regs.eax & 0xffff);
		return B_ERROR;
	}

	info.bits_per_gun = regs.ebx >> 8;
	return B_OK;
}


static status_t
vbe_set_bits_per_gun(vesa_info& info, uint8 bits)
{
	info.bits_per_gun = 6;

	bios_state* state;
	status_t status = vbe_call_prepare(&state);
	if (status != B_OK)
		return status;

	status = vbe_set_bits_per_gun(state, info, bits);

	vbe_call_finish(state);
	return status;
}


/*!	Remaps the frame buffer if necessary; if we've already mapped the complete
	frame buffer, there is no need to map it again.
*/
static status_t
remap_frame_buffer(vesa_info& info, addr_t physicalBase, uint32 width,
	uint32 height, int8 depth, uint32 bytesPerRow, bool initializing)
{
	vesa_shared_info& sharedInfo = *info.shared_info;
	addr_t frameBuffer = info.frame_buffer;

	if (!info.complete_frame_buffer_mapped) {
		addr_t base = physicalBase;
		size_t size = bytesPerRow * height;
		bool remap = !initializing;

		if (info.physical_frame_buffer_size != 0) {
			// we can map the complete frame buffer
			base = info.physical_frame_buffer;
			size = info.physical_frame_buffer_size;
			remap = true;
		}

		if (remap) {
			area_id area = map_physical_memory("vesa frame buffer", base,
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
			vm_set_area_memory_type(area, base, B_MTR_WC);

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
vesa_init(vesa_info& info)
{
	frame_buffer_boot_info* bufferInfo
		= (frame_buffer_boot_info*)get_boot_item(FRAME_BUFFER_BOOT_INFO, NULL);
	if (bufferInfo == NULL)
		return B_ERROR;

	info.vbe_capabilities = bufferInfo->vesa_capabilities;
	info.complete_frame_buffer_mapped = false;

	// Find out which PCI device we belong to, so that we know its frame buffer
	// size
	find_graphics_card(bufferInfo->physical_frame_buffer,
		info.physical_frame_buffer, info.physical_frame_buffer_size);

	size_t modesSize = 0;
	vesa_mode* modes = (vesa_mode*)get_boot_item(VESA_MODES_BOOT_INFO,
		&modesSize);
	info.modes = modes;

	size_t sharedSize = (sizeof(vesa_shared_info) + 7) & ~7;

	info.shared_area = create_area("vesa shared info",
		(void**)&info.shared_info, B_ANY_KERNEL_ADDRESS,
		ROUND_TO_PAGE_SIZE(sharedSize + modesSize), B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_CLONEABLE_AREA);
	if (info.shared_area < 0)
		return info.shared_area;

	vesa_shared_info& sharedInfo = *info.shared_info;

	memset(&sharedInfo, 0, sizeof(vesa_shared_info));

	if (modes != NULL) {
		sharedInfo.vesa_mode_offset = sharedSize;
		sharedInfo.vesa_mode_count = modesSize / sizeof(vesa_mode);

		memcpy((uint8*)&sharedInfo + sharedSize, modes, modesSize);
	}

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

	if (modes != NULL) {
		vbe_get_dpms_capabilities(info.vbe_dpms_capabilities,
			sharedInfo.dpms_capabilities);
		if (bufferInfo->depth <= 8)
			vbe_set_bits_per_gun(info, 8);
	}

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

	// Prepare BIOS environment
	bios_state* state;
	status_t status = vbe_call_prepare(&state);
	if (status != B_OK)
		return status;

	// Get mode information
	struct vbe_mode_info modeInfo;
	status = vbe_get_mode_info(state, info.modes[mode].mode, &modeInfo);
	if (status != B_OK) {
		dprintf(DEVICE_NAME": vesa_set_display_mode(): cannot get mode info\n");
		goto out;
	}

	// Set mode
	status = vbe_set_mode(state, info.modes[mode].mode);
	if (status != B_OK) {
		dprintf(DEVICE_NAME": vesa_set_display_mode(): cannot set mode\n");
		goto out;
	}

	if (info.modes[mode].bits_per_pixel <= 8)
		vbe_set_bits_per_gun(state, info, 8);

	// Map new frame buffer if necessary

	status = remap_frame_buffer(info, modeInfo.physical_base, modeInfo.width,
		modeInfo.height, modeInfo.bits_per_pixel, modeInfo.bytes_per_row,
		false);
	if (status == B_OK) {
		// Update shared frame buffer information
		info.shared_info->current_mode.virtual_width = modeInfo.width;
		info.shared_info->current_mode.virtual_height = modeInfo.height;
		info.shared_info->current_mode.space = get_color_space_for_depth(
			modeInfo.bits_per_pixel);
	}

out:
	vbe_call_finish(state);
	return status;
}


status_t
vesa_get_dpms_mode(vesa_info& info, uint32& mode)
{
	mode = B_DPMS_ON;
		// we always return a valid mode

	if (info.modes == NULL)
		return B_ERROR;

	// Prepare BIOS environment
	bios_state* state;
	status_t status = vbe_call_prepare(&state);
	if (status != B_OK)
		return status;

	bios_regs regs = {};
	regs.eax = 0x4f10;
	regs.ebx = 2;
	regs.esi = 0;
	regs.edi = 0;

	status = sBIOSModule->interrupt(state, 0x10, &regs);
	if (status != B_OK) {
		dprintf(DEVICE_NAME ": vesa_get_dpms_mode(): BIOS failed: %s\n",
			strerror(status));
		goto out;
	}

	if ((regs.eax & 0xffff) != 0x4f) {
		dprintf(DEVICE_NAME ": vesa_get_dpms_mode(): BIOS returned "
			"0x%" B_PRIx32 "\n", regs.eax & 0xffff);
		status = B_ERROR;
		goto out;
	}

	mode = vbe_to_system_dpms(regs.ebx >> 8);

out:
	vbe_call_finish(state);
	return status;
}


status_t
vesa_set_dpms_mode(vesa_info& info, uint32 mode)
{
	if (info.modes == NULL)
		return B_ERROR;

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

	// Prepare BIOS environment
	bios_state* state;
	status_t status = vbe_call_prepare(&state);
	if (status != B_OK)
		return status;

	bios_regs regs = {};
	regs.eax = 0x4f10;
	regs.ebx = (vbeMode << 8) | 1;
	regs.esi = 0;
	regs.edi = 0;

	status = sBIOSModule->interrupt(state, 0x10, &regs);
	if (status != B_OK) {
		dprintf(DEVICE_NAME ": vesa_set_dpms_mode(): BIOS failed: %s\n",
			strerror(status));
		goto out;
	}

	if ((regs.eax & 0xffff) != 0x4f) {
		dprintf(DEVICE_NAME ": vesa_set_dpms_mode(): BIOS returned "
			"0x%04" B_PRIx32 "\n", regs.eax & 0xffff);
		status = B_ERROR;
		goto out;
	}

out:
	vbe_call_finish(state);
	return status;
}


status_t
vesa_set_indexed_colors(vesa_info& info, uint8 first, uint8* colors,
	uint16 count)
{
	bios_regs regs = {};
	uint32 shift, physicalAddress;

	if (first + count > 256)
		count = 256 - first;

	// Prepare BIOS environment
	bios_state* state;
	status_t status = vbe_call_prepare(&state);
	if (status != B_OK)
		return status;

	uint8* palette = (uint8*)sBIOSModule->allocate_mem(state, 256 * 4);
	if (palette == NULL) {
		status = B_NO_MEMORY;
		goto out;
	}

	shift = 8 - info.bits_per_gun;

	// convert colors to VESA palette
	for (int32 i = first; i < count; i++) {
		uint8 color[3];
		if (user_memcpy(color, &colors[i * 3], 3) < B_OK) {
			status = B_BAD_ADDRESS;
			goto out;
		}

		// order is BGR-
		palette[i * 4 + 0] = color[2] >> shift;
		palette[i * 4 + 1] = color[1] >> shift;
		palette[i * 4 + 2] = color[0] >> shift;
		palette[i * 4 + 3] = 0;
	}

	// set palette
	physicalAddress = sBIOSModule->physical_address(state, palette);
	regs.eax = 0x4f09;
	regs.ebx = 0;
	regs.ecx = count;
	regs.edx = first;
	regs.es  = physicalAddress >> 4;
	regs.edi = physicalAddress - (regs.es << 4);

	status = sBIOSModule->interrupt(state, 0x10, &regs);
	if (status != B_OK) {
		dprintf(DEVICE_NAME ": vesa_set_indexed_colors(): BIOS failed: %s\n",
			strerror(status));
		goto out;
	}

	if ((regs.eax & 0xffff) != 0x4f) {
		dprintf(DEVICE_NAME ": vesa_set_indexed_colors(): BIOS returned "
			"0x%04" B_PRIx32 "\n", regs.eax & 0xffff);
		status = B_ERROR;
	}

out:
	vbe_call_finish(state);
	return status;
}
