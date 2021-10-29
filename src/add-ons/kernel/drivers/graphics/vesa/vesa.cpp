/*
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "vesa_private.h"
#include "vesa.h"

#define _GNU_SOURCE
#include <string.h>

#include <drivers/bios.h>

#include <boot_item.h>
#include <frame_buffer_console.h>
#include <util/kernel_cpp.h>
#include <vm/vm.h>

#include "driver.h"
#include "utility.h"
#include "vesa_info.h"

#include "atombios.h"


#define LINEAR_ADDRESS(segment, offset) \
	        (((addr_t)(segment) << 4) + (addr_t)(offset))
#define SEGMENTED_TO_LINEAR(segmented) \
	        LINEAR_ADDRESS((addr_t)(segmented) >> 16, (addr_t)(segmented) & 0xffff)


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
vbe_get_dpms_capabilities(bios_state* state, uint32& vbeMode, uint32& mode)
{
	// we always return a valid mode
	vbeMode = 0;
	mode = B_DPMS_ON;

	bios_regs regs = {};
	regs.eax = 0x4f10;
	regs.ebx = 0;
	regs.esi = 0;
	regs.edi = 0;

	status_t status = sBIOSModule->interrupt(state, 0x10, &regs);
	if (status != B_OK) {
		dprintf(DEVICE_NAME ": vbe_get_dpms_capabilities(): BIOS failed: %s\n",
			strerror(status));
		return status;
	}

	if ((regs.eax & 0xffff) != 0x4f) {
		dprintf(DEVICE_NAME ": vbe_get_dpms_capabilities(): BIOS returned "
			"0x%04" B_PRIx32 "\n", regs.eax & 0xffff);
		return B_ERROR;
	}

	vbeMode = regs.ebx >> 8;
	mode = vbe_to_system_dpms(vbeMode);
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
vbe_get_vesa_info(bios_state* state, vesa_info& info)
{
	vbe_info_block* infoHeader = (vbe_info_block*)sBIOSModule->allocate_mem(state, 256);
	phys_addr_t physicalAddress = sBIOSModule->physical_address(state, infoHeader);

	bios_regs regs = {};
	regs.eax = 0x4f00;
	regs.es  = physicalAddress >> 4;
	regs.edi = physicalAddress - (regs.es << 4);

	status_t status = sBIOSModule->interrupt(state, 0x10, &regs);
	if (status != B_OK) {
		dprintf(DEVICE_NAME ": vbe_get_vesa_info(): BIOS failed: %s\n",
			strerror(status));
		return status;
	}

	if ((regs.eax & 0xffff) != 0x4f) {
		dprintf(DEVICE_NAME ": vbe_get_vesa_info(): BIOS returned "
			"0x%04" B_PRIx32 "\n", regs.eax & 0xffff);
		return B_ERROR;
	}

	info.shared_info->vram_size = infoHeader->total_memory * 65536;
	strlcpy(info.shared_info->name, (char*)sBIOSModule->virtual_address(state,
			SEGMENTED_TO_LINEAR(infoHeader->oem_string)), 32);

	return status;
}


/*! Identify the BIOS type if it's one of the common ones, and locate where the BIOS store its
 * allowed video modes table. We can then patch this table to add extra video modes that the
 * manufacturer didn't allow.
 */
static void
vbe_identify_bios(bios_state* state, vesa_shared_info* sharedInfo)
{
	// Get a pointer to the BIOS
	const uintptr_t kBiosBase = 0xc0000;
	uint8_t* bios = (uint8_t*)sBIOSModule->virtual_address(state, kBiosBase);

	const size_t kAtomBiosHeaderOffset = 0x48;
	const char kAtomSignature[] = {'A', 'T', 'O', 'M'};

	ATOM_ROM_HEADER* atomRomHeader = (ATOM_ROM_HEADER*)(bios + kAtomBiosHeaderOffset);

	sharedInfo->bios_type = kUnknownBiosType;

	if (*(uint16*)(bios + 0x44) == 0x8086) {
		dprintf(DEVICE_NAME ": detected Intel BIOS\n");

		// TODO check if we can find the mode table
		sharedInfo->bios_type = kIntelBiosType;
	} else if (memcmp(atomRomHeader->uaFirmWareSignature,  kAtomSignature, 4) == 0) {
		dprintf(DEVICE_NAME ": detected ATOM BIOS\n");

		ATOM_MASTER_DATA_TABLE* masterDataTable = (ATOM_MASTER_DATA_TABLE*)(bios
			+ atomRomHeader->usMasterDataTableOffset);
		dprintf(DEVICE_NAME ": list of data tables: %p", &masterDataTable->ListOfDataTables);
		ATOM_ANALOG_TV_INFO* standardVesaTable = (ATOM_ANALOG_TV_INFO*)(bios
			+ masterDataTable->ListOfDataTables.StandardVESA_Timing);
		dprintf(DEVICE_NAME ": std_vesa: %p", standardVesaTable);
		sharedInfo->mode_table_offset = (uint8*)&standardVesaTable->aModeTimings - bios;

		size_t tableSize = standardVesaTable->sHeader.usStructureSize 
			- sizeof(ATOM_COMMON_TABLE_HEADER);
		if (tableSize % sizeof(ATOM_MODE_TIMING) == 0)
			sharedInfo->bios_type = kAtomBiosType2;
		else
			sharedInfo->bios_type = kAtomBiosType1;
	} else if (memmem(bios, 512, "NVID", 4) != NULL) {
		dprintf(DEVICE_NAME ": detected nVidia BIOS\n");

		// TODO check if we can find the mode table
		sharedInfo->bios_type = kNVidiaBiosType;
	} else {
		dprintf(DEVICE_NAME ": unknown BIOS type, custom video modes will not be available\n");
	}
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

	bios_state* state;
	status_t status = vbe_call_prepare(&state);
	if (status != B_OK)
		return status;

	vbe_identify_bios(state, &sharedInfo);

	vbe_get_dpms_capabilities(state, info.vbe_dpms_capabilities,
		sharedInfo.dpms_capabilities);
	if (bufferInfo->depth <= 8)
		vbe_set_bits_per_gun(state, info, 8);

	vbe_call_finish(state);

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
vbe_patch_intel_bios(bios_state* state, display_mode& mode)
{
	edid1_detailed_timing_raw timing;

	timing.pixel_clock = mode.timing.pixel_clock / 10;

	timing.h_active = mode.timing.h_display & 0xFF;
	timing.h_active_high = (mode.timing.h_display >> 8) & 0xF;

	uint16 h_blank = mode.timing.h_total - mode.timing.h_display;
	timing.h_blank = h_blank & 0xff;
	timing.h_blank_high = (h_blank >> 8) & 0xF;

	timing.v_active = mode.timing.v_display & 0xFF;
	timing.v_active_high = (mode.timing.v_display >> 8) & 0xF;

	uint16 v_blank = mode.timing.v_total - mode.timing.v_display;
	timing.v_blank = v_blank & 0xff;
	timing.v_blank_high = (v_blank >> 8) & 0xF;

	uint16 h_sync_off = mode.timing.h_sync_start - mode.timing.h_display;
	timing.h_sync_off = h_sync_off & 0xFF;
	timing.h_sync_off_high = (h_sync_off >> 8) & 0x3;

	uint16 h_sync_width = mode.timing.h_sync_end - mode.timing.h_sync_start;
	timing.h_sync_width = h_sync_width & 0xFF;
	timing.h_sync_width_high = (h_sync_width >> 8) & 0x3;

	uint16 v_sync_off = mode.timing.v_sync_start - mode.timing.v_display;
	timing.v_sync_off = v_sync_off & 0xF;
	timing.v_sync_off_high = (v_sync_off >> 4) & 0x3;

	uint16 v_sync_width = mode.timing.v_sync_end - mode.timing.v_sync_start;
	timing.v_sync_width = v_sync_width & 0xF;
	timing.v_sync_width_high = (v_sync_width >> 4) & 0x3;

	timing.h_size = 0;
	timing.v_size = 0;
	timing.h_size_high = 0;
	timing.v_size_high = 0;
	timing.h_border = 0;
	timing.v_border = 0;
	timing.interlaced = 0;
	timing.stereo = 0;
	timing.sync = 3;
	timing.misc = 0;
	timing.stereo_il = 0;

	static const uint8 knownMode[] = { 0x64, 0x19, 0x00, 0x40, 0x41, 0x00, 0x26, 0x30, 0x18,
		0x88, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18};
		// This is the EDID description for a standard 1024x768 timing. We will find and replace
		// all occurences of it in the BIOS with our custom mode.

	// Get a pointer to the BIOS
	const uintptr_t kBiosBase = 0xc0000;
	const size_t kBiosSize = 0x10000;
	uint8_t* bios = (uint8_t*)sBIOSModule->virtual_address(state, kBiosBase);
	uint8_t* biosEnd = bios + kBiosSize;

	int replacementCount = 0;
	while (bios < biosEnd) {
		bios = (uint8_t*)memmem(bios, biosEnd - bios, knownMode, sizeof(knownMode));
		if (bios == NULL)
			break;
		memcpy(bios, &timing, sizeof(timing));
		bios += sizeof(knownMode);
		replacementCount++;
	}

	dprintf(DEVICE_NAME ": patched custom mode in %d locations\n", replacementCount);

	// Did we manage to find a mode descriptor to replace?
	if (replacementCount == 0)
		return B_NOT_SUPPORTED;
	return B_OK;
}


status_t
vbe_patch_nvidia_bios(bios_state* state, display_mode& mode)
{
	struct nvidia_mode {
		uint32 width;
		uint32 height;
		uint8 patch0[17];
		uint8 patch1[9];
		uint8 patch2[13];
		uint8 patch3[5];
	};

	static const nvidia_mode allowedModes[] = {
		{1280,  720, {0x16, 0xCB, 0x9F, 0x9F, 0x8F, 0xA7, 0x17, 0xEA, 0xD2, 0xCF, 0xCF, 0xEB, 0x47,
				0xE0, 0xC0, 0x00, 0x01},
			{0x00, 0x05, 0xD0, 0x02, 0xA0, 0x2C, 0x10, 0x07, 0x05},
			{0x7B, 0x01, 0x03, 0x7B, 0x01, 0x08, 0x01, 0x20, 0x80, 0x02, 0xFF, 0xFF, 0x20},
			{0x00, 0x05, 0xBA, 0xD0, 0x02}},
		{1280,  800, {0x12, 0xCD, 0x9F, 0x9F, 0x91, 0xA9, 0x1A, 0x3A, 0x21, 0x1F, 0x1F, 0x3B, 0x44,
				0xFE, 0xC0, 0x00, 0x01},
			{0x00, 0x05, 0x20, 0x03, 0xA0, 0x32, 0x10, 0x23, 0x05},
			{0x61, 0x01, 0x03, 0x61, 0x01, 0x08, 0x01, 0x20, 0x80, 0x02, 0xFF, 0xFF, 0x20},
			{0x00, 0x05, 0xBA, 0x20, 0x03}},
		{1360,  768, {0x16, 0xB9, 0xA9, 0x9F, 0x8F, 0xB2, 0x16, 0x14, 0x01, 0xFF, 0xCF, 0xEB, 0x46,
				0xEA, 0xC0, 0x00, 0x01},
			{0x50, 0x05, 0x00, 0x03, 0xAA, 0x2F, 0x10, 0x07, 0x05},
			{0x4D, 0x01, 0x03, 0x4D, 0x01, 0x08, 0x01, 0x20, 0xA8, 0x02, 0xFF, 0xFF, 0x20},
			{0x50, 0x05, 0xBA, 0x00, 0x03}},
		{1400, 1050, {0x12, 0xE6, 0xAE, 0xAE, 0x8A, 0xBB, 0x8E, 0x3D, 0x1B, 0x19, 0x19, 0x3E, 0x0E,
				0x00, 0xC0, 0x24, 0x12},
			{0x78, 0x05, 0x1A, 0x04, 0xAF, 0x4A, 0x0E, 0x21, 0x05},
			{0x49, 0x01, 0x03, 0x49, 0x01, 0x08, 0x01, 0x20, 0xBC, 0x02, 0xFF, 0xFF, 0x20},
			{0x78, 0x05, 0xBA, 0x1A, 0x04}},
		{1440,  900, {0x12, 0xE9, 0xB3, 0xB3, 0x8D, 0xBF, 0x92, 0xA3, 0x85, 0x83, 0x83, 0xA4, 0x48,
				0xFE, 0xC0, 0x00, 0x00},
			{0xA0, 0x05, 0x84, 0x03, 0xB4, 0x38, 0x10, 0x24, 0x05},
			{0x65, 0x01, 0x03, 0x65, 0x01, 0x08, 0x01, 0x20, 0xD0, 0x02, 0xFF, 0xFF, 0x20},
			{0xA0, 0x05, 0xBA, 0x84, 0x03}},
		{1600,  900, {0x1A, 0xD7, 0xC7, 0xC7, 0x9B, 0xCD, 0x11, 0x9C, 0x86, 0x83, 0x83, 0x9D, 0x4B,
				0xFE, 0xC0, 0x00, 0x00},
			{0x40, 0x06, 0x84, 0x03, 0xC8, 0x38, 0x10, 0x27, 0x05},
			{0x67, 0x01, 0x03, 0x67, 0x01, 0x08, 0x01, 0x20, 0x20, 0x03, 0xFF, 0xFF, 0x20},
			{0x40, 0x06, 0xBA, 0x84, 0x03}},
		{1600, 1200, {0x12, 0x03, 0xC7, 0xC7, 0x87, 0xD1, 0x09, 0xE0, 0xB1, 0xAF, 0xAF, 0xE1, 0x04,
				0x00, 0x01, 0x24, 0x13},
			{0x40, 0x06, 0xB0, 0x04, 0xC8, 0x4A, 0x10, 0x19, 0x05},
			{0x4A, 0x01, 0x03, 0x4A, 0x01, 0x08, 0x01, 0x20, 0x20, 0x03, 0xFF, 0xFF, 0x20},
			{0x40, 0x06, 0xBA, 0xB0, 0x04}},
		{1680, 1050, {0x12, 0x15, 0xD1, 0xD1, 0x99, 0xE0, 0x17, 0x3D, 0x1B, 0x19, 0x19, 0x3E, 0x0E,
				0x00, 0x01, 0x24, 0x13},
			{0x90, 0x06, 0x1A, 0x04, 0xD2, 0x41, 0x10, 0x25, 0x05},
			{0x69, 0x01, 0x03, 0x69, 0x01, 0x08, 0x01, 0x20, 0x48, 0x03, 0xFF, 0xFF, 0x20},
			{0x90, 0x06, 0xBA, 0x1A, 0x04}},
		{1920, 1080, {0x16, 0x0E, 0xEF, 0x9F, 0x8F, 0xFD, 0x02, 0x63, 0x3B, 0x37, 0xCF, 0xEB, 0x40,
				0x00, 0xC1, 0x24, 0x02},
			{0x80, 0x07, 0x38, 0x04, 0xF0, 0x42, 0x10, 0x07, 0x05},
			{0x4D, 0x01, 0x03, 0x4D, 0x01, 0x08, 0x01, 0x20, 0xC0, 0x03, 0xFF, 0xFF, 0x20},
			{0x80, 0x07, 0xBA, 0x38, 0x04}},
		{1920, 1200, {0x12, 0x3F, 0xEF, 0xEF, 0x83, 0x01, 0x1B, 0xD8, 0xB1, 0xAF, 0xAF, 0xD9, 0x04,
				0x00, 0x41, 0x25, 0x12},
			{0x80, 0x07, 0xB0, 0x04, 0xF0, 0x4B, 0x10, 0x26, 0x05},
			{0x7D, 0x01, 0x03, 0x7D, 0x01, 0x08, 0x01, 0x20, 0xC0, 0x03, 0xFF, 0xFF, 0x20},
			{0x80, 0x07, 0xBA, 0xB0, 0x04}},
		{2048, 1536, {0x12, 0x63, 0xFF, 0xFF, 0x9D, 0x12, 0x0E, 0x34, 0x01, 0x00, 0x00, 0x35, 0x44,
				0xE0, 0x41, 0x25, 0x13},
			{0x00, 0x08, 0x00, 0x06, 0x00, 0x60, 0x10, 0x22, 0x05},
			{0x7A, 0x01, 0x03, 0x52, 0x01, 0x08, 0x01, 0x20, 0x00, 0x04, 0xFF, 0xFF, 0x20},
			{0x00, 0x08, 0xBA, 0x00, 0x06}}
	};

	static const nvidia_mode knownMode = { 0, 0,
		{0x34, 0x2d, 0x27, 0x28, 0x90, 0x2b, 0xa0, 0xbf, 0x9c, 0x8f, 0x96, 0xb9, 0x8e, 0x1f, 0x00,
			0x00, 0x00},
		{0x28, 0x00, 0x19, 0x00, 0x28, 0x18, 0x08, 0x08, 0x05},
		{0x82, 0x0f, 0x03, 0x01, 0x00, 0x00, 0x08, 0x04, 0x14, 0x00, 0x00, 0x08, 0x17},
		{0x40, 0x06, 0xba, 0xb0, 0x04}
	};

	int i;
	for (i = 0; i < B_COUNT_OF(allowedModes); i++) {
		if (allowedModes[i].width == mode.timing.h_display
			&& allowedModes[i].height == mode.timing.v_display) {
			break;
		}
	}

	if (i >= B_COUNT_OF(allowedModes))
		return B_BAD_VALUE;

	// Get a pointer to the BIOS
	const uintptr_t kBiosBase = 0xc0000;
	const size_t kBiosSize = 0x10000;
	uint8_t* biosBase = (uint8_t*)sBIOSModule->virtual_address(state, kBiosBase);
	uint8_t* biosEnd = bios + kBiosSize

	int replacementCount = 0;
	uint8_t* bios = biosBase;
	while (bios < biosEnd) {
		bios = (uint8_t*)memmem(bios, biosEnd - bios, knownMode.patch0, sizeof(knownMode.patch0));
		if (bios == NULL)
			break;
		memcpy(bios, allowedModes[i].patch0, sizeof(allowedModes[i].patch0));
		bios += sizeof(knownMode.patch0);
		replacementCount++;
	}
	dprintf(DEVICE_NAME ": applied patch0 in %d locations\n", replacementCount);

	replacementCount = 0;
	bios = biosBase;
	while (bios < biosEnd) {
		bios = (uint8_t*)memmem(bios, biosEnd - bios, knownMode.patch1, sizeof(knownMode.patch1));
		if (bios == NULL)
			break;
		memcpy(bios, allowedModes[i].patch1, sizeof(allowedModes[i].patch1));
		bios += sizeof(knownMode.patch1);
		replacementCount++;
	}
	dprintf(DEVICE_NAME ": applied patch1 in %d locations\n", replacementCount);

	replacementCount = 0;
	bios = biosBase;
	while (bios < biosEnd) {
		bios = (uint8_t*)memmem(bios, biosEnd - bios, knownMode.patch2, sizeof(knownMode.patch2));
		if (bios == NULL)
			break;
		memcpy(bios, allowedModes[i].patch2, sizeof(allowedModes[i].patch2));
		bios += sizeof(knownMode.patch2);
		replacementCount++;
	}
	dprintf(DEVICE_NAME ": applied patch1 in %d locations\n", replacementCount);

	replacementCount = 0;
	bios = biosBase;
	while (bios < biosEnd) {
		bios = (uint8_t*)memmem(bios, biosEnd - bios, knownMode.patch3, sizeof(knownMode.patch3));
		if (bios == NULL)
			break;
		memcpy(bios, allowedModes[i].patch3, sizeof(allowedModes[i].patch3));
		bios += sizeof(knownMode.patch3);
		replacementCount++;
	}
	dprintf(DEVICE_NAME ": applied patch1 in %d locations\n", replacementCount);

	if ((bios[0x34] & 0x8F) == 0x80)
		bios[0x34] |= 0x01;

	return B_OK;
}


status_t
vbe_patch_atom1_bios(vesa_info& info, bios_state* state, display_mode& mode)
{
	// Get a pointer to the BIOS
	const uintptr_t kBiosBase = 0xc0000;
	uint8_t* bios = (uint8_t*)sBIOSModule->virtual_address(state, kBiosBase);

	ATOM_MODE_TIMING* timing = (ATOM_MODE_TIMING*)(bios + info.shared_info->mode_table_offset);
	dprintf(DEVICE_NAME ": patching ATOM mode timing (overwriting mode %dx%d)\n",
		timing->usCRTC_H_Disp, timing->usCRTC_V_Disp);

	timing->usCRTC_H_Total = mode.timing.h_total;
	timing->usCRTC_H_Disp = mode.timing.h_display;
	timing->usCRTC_H_SyncStart = mode.timing.h_sync_start;
	timing->usCRTC_H_SyncWidth = mode.timing.h_sync_end - mode.timing.h_sync_start;

	timing->usCRTC_V_Total = mode.timing.v_total;
	timing->usCRTC_V_Disp = mode.timing.v_display;
	timing->usCRTC_V_SyncStart = mode.timing.v_sync_start;
	timing->usCRTC_V_SyncWidth = mode.timing.v_sync_end - mode.timing.v_sync_start;

	timing->usPixelClock = mode.timing.pixel_clock / 10;

	return B_OK;
}


status_t
vbe_patch_atom2_bios(vesa_info& info, bios_state* state, display_mode& mode)
{
	// Get a pointer to the BIOS
	const uintptr_t kBiosBase = 0xc0000;
	uint8_t* bios = (uint8_t*)sBIOSModule->virtual_address(state, kBiosBase);

	ATOM_DTD_FORMAT* timing = (ATOM_DTD_FORMAT*)(bios + info.shared_info->mode_table_offset);
	dprintf(DEVICE_NAME ": patching ATOM DTD format (overwriting mode %dx%d)\n",
		timing->usHActive, timing->usVActive);

	timing->usHBlanking_Time = mode.timing.h_total - mode.timing.h_display;
	timing->usHActive = mode.timing.h_display;
	timing->usHSyncOffset = mode.timing.h_sync_start;
	timing->usHSyncWidth = mode.timing.h_sync_end - mode.timing.h_sync_start;

	timing->usVBlanking_Time = mode.timing.v_total - mode.timing.v_display;
	timing->usVActive = mode.timing.v_display;
	timing->usVSyncOffset = mode.timing.v_sync_start;
	timing->usVSyncWidth = mode.timing.v_sync_end - mode.timing.v_sync_start;

	timing->usPixClk = mode.timing.pixel_clock / 10;

	return B_OK;
}


status_t
vesa_set_custom_display_mode(vesa_info& info, display_mode& mode)
{
	if (info.shared_info->bios_type == kUnknownBiosType)
		return B_NOT_SUPPORTED;

	// Prepare BIOS environment
	bios_state* state;
	status_t status = vbe_call_prepare(&state);
	if (status != B_OK)
		return status;

	int32 modeIndex; // Index of the mode that will be patched

	// Patch bios to inject custom video mode
	switch (info.shared_info->bios_type) {
		case kIntelBiosType:
			status = vbe_patch_intel_bios(state, mode);
			// The patch replaces the 1024x768 modes with our custom resolution. We can then use
			// mode 0x118 which is the standard VBE2 mode for 1024x768 at 32 bits per pixel, and
			// know it will use our new timings.
			// TODO use modes 105 (256 colors), 116 (15 bit) and 117 (16 bit) depending on the
			// requested colorspace. They should work too.
			// TODO ideally locate the 1024x768 modes from the mode list in info.modes[], instead
			// of hardcoding its number, in case the BIOS does not use VBE2 standard numbering.
			modeIndex = 0x118;
			break;
#if 0
		case kNVidiaBiosType:
			status = vbe_patch_nvidia_bios(state, mode);
			break;
#endif
		case kAtomBiosType1:
			status = vbe_patch_atom1_bios(info, state, mode);
			modeIndex = 0; // TODO how does this work? Is it 100 (first VBE2 mode)?
			break;
		case kAtomBiosType2:
			status = vbe_patch_atom2_bios(info, state, mode);
			modeIndex = 0; // TODO how does this work? Is it 100 (first VBE2 mode)?
			break;
		default:
			status = B_NOT_SUPPORTED;
			break;
	}

	if (status != B_OK)
		goto out;

	// Get mode information
	struct vbe_mode_info modeInfo;
	for (int i = 0; i < info.shared_info->mode_count; i++) {
		status = vbe_get_mode_info(state, info.modes[i].mode, &modeInfo);
		if (status != B_OK) {
			// Sometimes the patching prevents us from getting the mode info?
			dprintf(DEVICE_NAME ": vesa_set_custom_display_mode(): cannot get mode info for %x\n",
				info.modes[i].mode);
			// Just ignore modes that turn out to be invalid...
			continue;
		}

		if (modeInfo.width == mode.timing.h_display && modeInfo.height == mode.timing.v_display
			&& get_color_space_for_depth(info.modes[i].bits_per_pixel) == mode.space) {
			modeIndex = info.modes[i].mode;
			break;
		}
	}

	if (modeIndex >= 0) {
		dprintf(DEVICE_NAME ": custom mode resolution %dx%d succesfully patched at index %"
			B_PRIx32 "\n", modeInfo.width, modeInfo.height, modeIndex);
	} else {
		dprintf(DEVICE_NAME ": video mode patching failed!\n");
		goto out;
	}

	dprintf(DEVICE_NAME ": custom mode resolution %dx%d\n", modeInfo.width, modeInfo.height);

	// Set mode
	status = vbe_set_mode(state, modeIndex);
	if (status != B_OK) {
		dprintf(DEVICE_NAME ": vesa_set_custom_display_mode(): cannot set mode\n");
		goto out;
	}

	if (info.modes[modeIndex].bits_per_pixel <= 8)
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
