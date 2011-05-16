/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Support for i915 chipset and up based on the X driver,
 * Copyright 2006-2007 Intel Corporation.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Alexander von Gluck, kallisti5@unixzen.com
 */


#include "accelerant_protos.h"
#include "accelerant.h"
#include "utility.h"
#include "mode.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <create_display_modes.h>


#define TRACE_MODE
#ifdef TRACE_MODE
extern "C" void _sPrintf(const char *format, ...);
#	define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#	define TRACE(x...) ;
#endif


status_t
create_mode_list(void)
{
	const color_space kRadeonHDSpaces[] = {B_RGB32_LITTLE, B_RGB24_LITTLE,
		B_RGB16_LITTLE, B_RGB15_LITTLE, B_CMAP8};

	gInfo->mode_list_area = create_display_modes("radeon HD modes",
		gInfo->shared_info->has_edid ? &gInfo->shared_info->edid_info : NULL,
		NULL, 0, kRadeonHDSpaces,
		sizeof(kRadeonHDSpaces) / sizeof(kRadeonHDSpaces[0]),
		is_mode_supported, &gInfo->mode_list, &gInfo->shared_info->mode_count);
	if (gInfo->mode_list_area < B_OK)
		return gInfo->mode_list_area;

	gInfo->shared_info->mode_list_area = gInfo->mode_list_area;

	return B_OK;
}


//	#pragma mark -


uint32
radeon_accelerant_mode_count(void)
{
	TRACE("%s\n", __func__);

	return gInfo->shared_info->mode_count;
}


status_t
radeon_get_mode_list(display_mode *modeList)
{
	TRACE("%s\n", __func__);
	memcpy(modeList, gInfo->mode_list,
		gInfo->shared_info->mode_count * sizeof(display_mode));
	return B_OK;
}


status_t
radeon_get_edid_info(void* info, size_t size, uint32* edid_version)
{
	TRACE("%s\n", __func__);
	if (!gInfo->shared_info->has_edid)
		return B_ERROR;
	if (size < sizeof(struct edid1_info))
		return B_BUFFER_OVERFLOW;

	memcpy(info, &gInfo->shared_info->edid_info, sizeof(struct edid1_info));
	*edid_version = EDID_VERSION_1;

	return B_OK;
}


static void
get_color_space_format(const display_mode &mode, uint32 &colorMode,
	uint32 &bytesPerRow, uint32 &bitsPerPixel)
{
	uint32 bytesPerPixel;

	switch (mode.space) {
		case B_RGB32_LITTLE:
			colorMode = DISPLAY_CONTROL_RGB32;
			bytesPerPixel = 4;
			bitsPerPixel = 32;
			break;
		case B_RGB16_LITTLE:
			colorMode = DISPLAY_CONTROL_RGB16;
			bytesPerPixel = 2;
			bitsPerPixel = 16;
			break;
		case B_RGB15_LITTLE:
			colorMode = DISPLAY_CONTROL_RGB15;
			bytesPerPixel = 2;
			bitsPerPixel = 15;
			break;
		case B_CMAP8:
		default:
			colorMode = DISPLAY_CONTROL_CMAP8;
			bytesPerPixel = 1;
			bitsPerPixel = 8;
			break;
	}

	bytesPerRow = mode.virtual_width * bytesPerPixel;

	// Make sure bytesPerRow is a multiple of 64
	// TODO: check if the older chips have the same restriction!
	if ((bytesPerRow & 63) != 0)
		bytesPerRow = (bytesPerRow + 63) & ~63;
}


// Blacks the screen out, useful for mode setting
static void
CardBlankSet(int crtNumber, bool blank)
{
	int blackColorReg;
	int blankControlReg;

	if (crtNumber == 2) {
		blackColorReg = D2CRTC_BLACK_COLOR;
		blankControlReg = D2CRTC_BLANK_CONTROL;
	} else {
		blackColorReg = D1CRTC_BLACK_COLOR;
		blankControlReg = D1CRTC_BLANK_CONTROL;
	}

	write32(blackColorReg, 0);
	write32AtMask(blankControlReg, blank ? 0x00000100 : 0, 0x00000100);
}


static void
CardFBSet(int crtNumber, display_mode *mode)
{
	uint16_t chipset = gInfo->shared_info->device_chipset;

	uint32 colorMode;
	uint32 bytesPerRow;
	uint32 bitsPerPixel;

	// Our registers
	// (set to 0 to avoid reading/writing random memory if not set)
	uint16_t regOffset = 0;
	uint16_t grphEnable = 0;
	uint16_t grphControl = 0;
	uint16_t grphSwapControl = 0;
	uint16_t grphPrimarySurfaceAddr = 0;
	uint16_t grphPitch = 0;
	uint16_t grphSurfaceOffsetX = 0;
	uint16_t grphSurfaceOffsetY = 0;
	uint16_t grphXStart = 0;
	uint16_t grphYStart = 0;
	uint16_t grphXEnd = 0;
	uint16_t grphYEnd = 0;
	uint16_t grphDesktopHeight = 0;

	get_color_space_format(*mode, colorMode, bytesPerRow, bitsPerPixel);

	if (chipset >= RADEON_R800) {
		// Evergreen registers differ from r600-r700
		regOffset = (crtNumber == 2)
			? EVERGREEN_CRTC1_REGISTER_OFFSET : EVERGREEN_CRTC0_REGISTER_OFFSET;
		grphEnable = EVERGREEN_GRPH_ENABLE;
		grphControl = EVERGREEN_GRPH_CONTROL;
		grphSwapControl = EVERGREEN_GRPH_SWAP_CONTROL;
		grphPrimarySurfaceAddr = EVERGREEN_GRPH_PRIMARY_SURFACE_ADDRESS;
		grphPitch = EVERGREEN_GRPH_PITCH;
		grphSurfaceOffsetX = EVERGREEN_GRPH_SURFACE_OFFSET_X;
		grphSurfaceOffsetY = EVERGREEN_GRPH_SURFACE_OFFSET_Y;
		grphXStart = EVERGREEN_GRPH_X_START;
		grphYStart = EVERGREEN_GRPH_Y_START;
		grphXEnd = EVERGREEN_GRPH_X_END;
		grphYEnd = EVERGREEN_GRPH_Y_END;
		grphDesktopHeight = EVERGREEN_DESKTOP_HEIGHT;
	} else {
		// r600-r700 registers
		regOffset = (crtNumber == 2) ? D2_REG_OFFSET : D1_REG_OFFSET;
		grphEnable = D1GRPH_ENABLE;
		grphControl = D1GRPH_CONTROL;
		grphSwapControl = D1GRPH_SWAP_CNTL;
		grphPrimarySurfaceAddr = D1GRPH_PRIMARY_SURFACE_ADDRESS;
		grphPitch = D1GRPH_PITCH;
		grphSurfaceOffsetX = D1GRPH_SURFACE_OFFSET_X;
		grphSurfaceOffsetY = D1GRPH_SURFACE_OFFSET_Y;
		grphXStart = D1GRPH_X_START;
		grphYStart = D1GRPH_Y_START;
		grphXEnd = D1GRPH_X_END;
		grphYEnd = D1GRPH_Y_END;
		grphDesktopHeight = D1MODE_DESKTOP_HEIGHT;
	}

	// disable R/B swap, disable tiling, disable 16bit alpha, etc.
	write32AtMask(regOffset + grphEnable, 1, 0x00000001);
	write32(regOffset + grphControl, 0);

	switch (mode->space) {
		case B_CMAP8:
			write32AtMask(regOffset + grphControl, 0, 0x00000703);
			break;
		case B_RGB15_LITTLE:
			write32AtMask(regOffset + grphControl, 0x000001, 0x00000703);
			break;
		case B_RGB16_LITTLE:
			write32AtMask(regOffset + grphControl, 0x000101, 0x00000703);
			break;
		case B_RGB24_LITTLE:
		case B_RGB32_LITTLE:
		default:
			write32AtMask(regOffset + grphControl, 0x000002, 0x00000703);
			break;
	}

	write32(regOffset + grphSwapControl, 0);
		// only for chipsets > r600
		// R5xx - RS690 case is GRPH_CONTROL bit 16

	uint32 fbIntAddress = read32(R6XX_CONFIG_FB_BASE);
	uint32 fbOffset = gInfo->shared_info->frame_buffer_offset;

	write32(regOffset + grphPrimarySurfaceAddr,
		fbOffset + fbIntAddress);

	write32(regOffset + grphPitch, bytesPerRow / 4);
	write32(regOffset + grphSurfaceOffsetX, 0);
	write32(regOffset + grphSurfaceOffsetY, 0);
	write32(regOffset + grphXStart, 0);
	write32(regOffset + grphYStart, 0);
	write32(regOffset + grphXEnd, mode->virtual_width);
	write32(regOffset + grphYEnd, mode->virtual_height);

	/* D1Mode registers */
	write32(regOffset + grphDesktopHeight, mode->virtual_height);

	// update shared info
	gInfo->shared_info->bytes_per_row = bytesPerRow;
	gInfo->shared_info->current_mode = *mode;
	gInfo->shared_info->bits_per_pixel = bitsPerPixel;
}


static void
CardModeSet(int crtNumber, display_mode *mode)
{
	uint16_t chipset = gInfo->shared_info->device_chipset;

	uint16_t regOffset = 0;
	uint16_t grphControl = 0;

	if (chipset >= RADEON_R800) {
		// Evergreen registers differ from r600-r700
		regOffset = (crtNumber == 2)
			? EVERGREEN_CRTC1_REGISTER_OFFSET : EVERGREEN_CRTC0_REGISTER_OFFSET;
		grphControl = EVERGREEN_GRPH_CONTROL;
	} else {
		// r600-r700 registers
		regOffset = (crtNumber == 2) ? D2_REG_OFFSET : D1_REG_OFFSET;
		grphControl = D1GRPH_CONTROL;
	}

	CardBlankSet(crtNumber, true);

	display_timing& displayTiming = mode->timing;

	TRACE("%s called to do %dx%d\n",
		__func__, displayTiming.h_display, displayTiming.v_display);

	// enable read requests
	write32AtMask(regOffset + grphControl, 0, 0x01000000);

	// *** Horizontal
	write32(regOffset + D1CRTC_H_TOTAL, displayTiming.h_total - 1);

	// determine blanking based on passed modeline
	uint16 blankStart = displayTiming.h_display + 1;
	uint16 blankEnd = displayTiming.h_total;

	write32(regOffset + D1CRTC_H_BLANK_START_END,
		blankStart | (blankEnd << 16));

	write32(regOffset + D1CRTC_H_SYNC_A,
		(displayTiming.h_sync_end - displayTiming.h_sync_start) << 16);

	// set flag for neg. H sync
	if (!(displayTiming.flags & B_POSITIVE_HSYNC))
		write32(regOffset + D1CRTC_H_SYNC_A_CNTL, 0x01);

	// *** Vertical
	write32(regOffset + D1CRTC_V_TOTAL, displayTiming.v_total - 1);

	blankStart = displayTiming.v_display;
	blankEnd = displayTiming.v_total;

	write32(regOffset + D1CRTC_V_BLANK_START_END,
		blankStart | (blankEnd << 16));

	// Set Interlace if specified within mode line
	if (displayTiming.flags & B_TIMING_INTERLACED) {
		write32(regOffset + D1CRTC_INTERLACE_CONTROL, 0x1);
		write32(regOffset + D1MODE_DATA_FORMAT, 0x1);
	} else {
		write32(regOffset + D1CRTC_INTERLACE_CONTROL, 0x0);
		write32(regOffset + D1MODE_DATA_FORMAT, 0x0);
	}

	write32(regOffset + D1CRTC_V_SYNC_A,
		(displayTiming.v_sync_end - displayTiming.v_sync_start) << 16);

	// set flag for neg. V sync
	if (!(displayTiming.flags & B_POSITIVE_VSYNC))
		write32(regOffset + D1CRTC_V_SYNC_A_CNTL, 0x01);

	/*	set D1CRTC_HORZ_COUNT_BY2_EN to 0;
		should only be set to 1 on 30bpp DVI modes
	*/
	write32AtMask(regOffset + D1CRTC_COUNT_CONTROL, 0x0, 0x1);

	CardBlankSet(crtNumber, false);
}


static void
CardModeScale(int crtNumber, display_mode *mode)
{
	uint16_t regOffset = (crtNumber == 2) ? D2_REG_OFFSET : D1_REG_OFFSET;

	/* D1Mode registers */
	write32(regOffset + D1MODE_VIEWPORT_SIZE,
		mode->timing.v_display | (mode->timing.h_display << 16));
	write32(regOffset + D1MODE_VIEWPORT_START, 0);

/*  write32(regOffset + D1MODE_EXT_OVERSCAN_LEFT_RIGHT,
		(Overscan.OverscanLeft << 16) | Overscan.OverscanRight);
	write32(regOffset + D1MODE_EXT_OVERSCAN_TOP_BOTTOM,
		(Overscan.OverscanTop << 16) | Overscan.OverscanBottom);
*/
	write32(regOffset + D1SCL_ENABLE, 0);
	write32(regOffset + D1SCL_TAP_CONTROL, 0);
	write32(regOffset + D1MODE_CENTER, 2);
}


status_t
radeon_set_display_mode(display_mode *mode)
{
	int crtNumber = 1;

	CardFBSet(crtNumber, mode);
	CardModeSet(crtNumber, mode);
	CardModeScale(crtNumber, mode);

	return B_OK;
}


status_t
radeon_get_display_mode(display_mode *_currentMode)
{
	TRACE("%s\n", __func__);

	*_currentMode = gInfo->shared_info->current_mode;
	return B_OK;
}


status_t
radeon_get_frame_buffer_config(frame_buffer_config *config)
{
	TRACE("%s\n", __func__);

	uint32 offset = gInfo->shared_info->frame_buffer_offset;

	config->frame_buffer = gInfo->shared_info->graphics_memory + offset;
	config->frame_buffer_dma
		= (uint8 *)gInfo->shared_info->physical_graphics_memory + offset;
	config->bytes_per_row = gInfo->shared_info->bytes_per_row;

	return B_OK;
}


status_t
radeon_get_pixel_clock_limits(display_mode *mode, uint32 *_low, uint32 *_high)
{
	TRACE("%s\n", __func__);
/*
	if (_low != NULL) {
		// lower limit of about 48Hz vertical refresh
		uint32 totalClocks = (uint32)mode->timing.h_total * (uint32)mode->timing.v_total;
		uint32 low = (totalClocks * 48L) / 1000L;
		if (low < gInfo->shared_info->pll_info.min_frequency)
			low = gInfo->shared_info->pll_info.min_frequency;
		else if (low > gInfo->shared_info->pll_info.max_frequency)
			return B_ERROR;

		*_low = low;
	}

	if (_high != NULL)
		*_high = gInfo->shared_info->pll_info.max_frequency;
*/
	*_low = 48L;
	*_high = 100 * 1000000L;
	return B_OK;
}


bool
is_mode_supported(display_mode *mode)
{
	if (is_mode_sane(mode) != B_OK)
		return false;

	// TODO : Check if mode is supported on monitor

	return true;
}


/*
 * A quick sanity check of the provided display_mode
 */
status_t
is_mode_sane(display_mode *mode)
{
	// horizontal timing
	// validate h_sync_start is less then h_sync_end
	if (mode->timing.h_sync_start > mode->timing.h_sync_end) {
		TRACE("%s: ERROR: (%dx%d) "
			"received h_sync_start greater then h_sync_end!\n",
			__func__, mode->timing.h_display, mode->timing.v_display);
		return B_ERROR;
	}
	// validate h_total is greater then h_display
	if (mode->timing.h_total < mode->timing.h_display) {
		TRACE("%s: ERROR: (%dx%d) "
			"received h_total greater then h_display!\n",
			__func__, mode->timing.h_display, mode->timing.v_display);
		return B_ERROR;
	}

	// vertical timing
	// validate v_start is less then v_end
	if (mode->timing.v_sync_start > mode->timing.v_sync_end) {
		TRACE("%s: ERROR: (%dx%d) "
			"received v_sync_start greater then v_sync_end!\n",
			__func__, mode->timing.h_display, mode->timing.v_display);
		return B_ERROR;
	}
	// validate v_total is greater then v_display
	if (mode->timing.v_total < mode->timing.v_display) {
		TRACE("%s: ERROR: (%dx%d) "
			"received v_total greater then v_display!\n",
			__func__, mode->timing.h_display, mode->timing.v_display);
		return B_ERROR;
	}

	// calculate refresh rate for given timings to whole int (in Hz)
	int refresh = mode->timing.pixel_clock * 1000
		/ (mode->timing.h_total * mode->timing.v_total);

	if (refresh < 30 || refresh > 250) {
		TRACE("%s: ERROR: (%dx%d) "
			"refresh rate of %dHz is unlikely for any kind of monitor!\n",
			__func__, mode->timing.h_display, mode->timing.v_display, refresh);
		return B_ERROR;
	}

	return B_OK;
}

