/*
 * Copyright 2021, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under the terms of the MIT License.
 */


#include "vesa_private.h"
#include "vesa.h"

#include <SupportDefs.h>

#define _DEFAULT_SOURCE
#include <string.h>

#include "atombios.h"
#include "driver.h"
#include "edid_raw.h"
#include "utility.h"
#include "vesa_info.h"


extern bios_module_info* sBIOSModule;


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
		bios += sizeof(timing);
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

	size_t i;
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
	uint8_t* biosEnd = biosBase + kBiosSize;

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
	dprintf(DEVICE_NAME ": applied patch2 in %d locations\n", replacementCount);

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
	dprintf(DEVICE_NAME ": applied patch3 in %d locations\n", replacementCount);

	if ((biosBase[0x34] & 0x8F) == 0x80)
		biosBase[0x34] |= 0x01;

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


/*! Identify the BIOS type if it's one of the common ones, and locate where the BIOS store its
 * allowed video modes table. We can then patch this table to add extra video modes that the
 * manufacturer didn't allow.
 */
void
vesa_identify_bios(bios_state* state, vesa_shared_info* sharedInfo)
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


/*! Set a custom display mode by live patching the VESA BIOS to insert it in the modeline table.
 *
 * This is only supported for some video cards, where the format of the mode lines is known.
 */
status_t
vesa_set_custom_display_mode(vesa_info& info, display_mode& mode)
{
	int32 modeIndex = -1;
	int32 brokenModeIndex = -1;

	if (info.shared_info->bios_type == kUnknownBiosType)
		return B_NOT_SUPPORTED;

	// Prepare BIOS environment
	bios_state* state;
	status_t status = vbe_call_prepare(&state);
	if (status != B_OK)
		return status;

	// Patch bios to inject custom video mode
	switch (info.shared_info->bios_type) {
		case kIntelBiosType:
			status = vbe_patch_intel_bios(state, mode);
			// The patch replaces the 1024x768 modes with our custom resolution. We can then use
			// mode 0x118 which is the standard VBE2 mode for 1024x768 at 32 bits per pixel, and
			// know it will use our new timings.
			break;
		case kNVidiaBiosType:
			status = vbe_patch_nvidia_bios(state, mode);
			break;
		case kAtomBiosType1:
			status = vbe_patch_atom1_bios(info, state, mode);
			break;
		case kAtomBiosType2:
			status = vbe_patch_atom2_bios(info, state, mode);
			break;
		default:
			status = B_NOT_SUPPORTED;
			break;
	}

	if (status != B_OK)
		goto out;

	// The patching modified some mode, but we don't know which one. So we need to rescan the mode
	// list to find the correct one.
	struct vbe_mode_info modeInfo;
	for (uint32 i = 0; i < info.shared_info->vesa_mode_count; i++) {
		status = vbe_get_mode_info(state, info.modes[i].mode, &modeInfo);
		if (status != B_OK) {
			// Sometimes the patching prevents us from getting the mode info. The modesetting
			// still works, so we can detect the "broken" mode this way and then activate it.
			dprintf(DEVICE_NAME ": vesa_set_custom_display_mode(): cannot get mode info for %x\n",
				info.modes[i].mode);
			brokenModeIndex = info.modes[i].mode;
			continue;
		}

		if (modeInfo.width == mode.timing.h_display && modeInfo.height == mode.timing.v_display
			&& get_color_space_for_depth(info.modes[i].bits_per_pixel) == mode.space) {
			modeIndex = info.modes[i].mode;
			break;
		}
	}

	if (modeIndex < 0)
		modeIndex = brokenModeIndex;

	if (modeIndex >= 0) {
		dprintf(DEVICE_NAME ": custom mode resolution %dx%d succesfully patched at index %"
			B_PRIx32 "\n", modeInfo.width, modeInfo.height, modeIndex);
	} else {
		dprintf(DEVICE_NAME ": video mode patching failed!\n");
		goto out;
	}

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


