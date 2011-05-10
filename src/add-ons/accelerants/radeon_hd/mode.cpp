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

	// TODO : Read EDID for create_display_modes via AtomBios as well

	gInfo->mode_list_area = create_display_modes("radeon HD modes",
		gInfo->shared_info->has_edid ? &gInfo->shared_info->edid_info : NULL,
		NULL, 0, kRadeonHDSpaces,
		sizeof(kRadeonHDSpaces) / sizeof(kRadeonHDSpaces[0]),
		is_mode_supported, &gInfo->mode_list, &gInfo->shared_info->mode_count);
	if (gInfo->mode_list_area < B_OK)
		return gInfo->mode_list_area;

	gInfo->shared_info->mode_list_area = gInfo->mode_list_area;

	/* Example nonstandard mode line (1600x900@60) */
	//static display_mode sDisplayMode;
	//sDisplayMode.timing.pixel_clock =  97750;
	//sDisplayMode.timing.h_display = 1600;
	//sDisplayMode.timing.h_sync_start = 1648;
	//sDisplayMode.timing.h_sync_end = 1680;
	//sDisplayMode.timing.h_total = 1760;
	//sDisplayMode.timing.v_display = 900;
	//sDisplayMode.timing.v_sync_start = 903;
	//sDisplayMode.timing.v_sync_end = 908;
	//sDisplayMode.timing.v_total = 926;
	//sDisplayMode.timing.flags = B_POSITIVE_HSYNC;
		// Polarity

	//sDisplayMode.space = B_RGB32_LITTLE;
		// Pixel configuration
	//sDisplayMode.virtual_width = 1600;
	//sDisplayMode.virtual_height = 900;
	//sDisplayMode.h_display_start = 0;
	//sDisplayMode.v_display_start = 0;
	//sDisplayMode.flags = 0;
		// Mode flags (Some drivers use this)

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


static void
CardModeSet(display_mode *mode)
{
	// For now we assume D1
	uint32 regOffset = D1_REG_OFFSET;

	display_timing& displayTiming = mode->timing;

	TRACE("%s called to do %dx%d\n",
		__func__, displayTiming.h_display, displayTiming.v_display);

	// enable read requests
	write32AtMask(regOffset + D1CRTC_CONTROL, 0, 0x01000000);

	// *** Horizontal
	write32(regOffset + D1CRTC_H_TOTAL, displayTiming.h_total - 1);

	// determine blanking based on passed modeline
	uint16 blankStart = displayTiming.h_display;
	uint16 blankEnd = displayTiming.h_total;

	write32(regOffset + D1CRTC_H_BLANK_START_END,
		blankStart | (blankEnd << 16));

	write32(regOffset + D1CRTC_H_SYNC_A,
		(displayTiming.h_sync_end - displayTiming.h_sync_start) << 16);

	// set flag for neg. H sync
	if (displayTiming.flags & ~B_POSITIVE_HSYNC)
		write32(regOffset + D1CRTC_V_SYNC_A_CNTL, 0x01);

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
	if (displayTiming.flags & ~B_POSITIVE_VSYNC)
		write32(regOffset + D1CRTC_V_SYNC_A_CNTL, 0x02);

	/*	set D1CRTC_HORZ_COUNT_BY2_EN to 0;
		should only be set to 1 on 30bpp DVI modes
	*/
	write32AtMask(regOffset + D1CRTC_COUNT_CONTROL, 0x0, 0x1);

}


static void
CardModeScale(display_mode *mode)
{
	uint32 regOffset = D1_REG_OFFSET;

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
	CardModeSet(mode);
	CardModeScale(mode);

	uint32 colorMode, bytesPerRow, bitsPerPixel;
	get_color_space_format(*mode, colorMode, bytesPerRow, bitsPerPixel);

	uint32 regOffset = D1_REG_OFFSET;

	write32AtMask(regOffset + D1GRPH_ENABLE, 1, 0x00000001);

	/* disable R/B swap, disable tiling, disable 16bit alpha, etc. */
	write32(regOffset + D1GRPH_CONTROL, 0);

	switch (mode->space) {
	case B_CMAP8:
		write32AtMask(regOffset + D1GRPH_CONTROL, 0, 0x00000703);
		break;
	case B_RGB15_LITTLE:
		write32AtMask(regOffset + D1GRPH_CONTROL, 0x000001, 0x00000703);
		break;
	case B_RGB16_LITTLE:
		write32AtMask(regOffset + D1GRPH_CONTROL, 0x000101, 0x00000703);
		break;
	case B_RGB24_LITTLE:
	case B_RGB32_LITTLE:
	default:
		write32AtMask(regOffset + D1GRPH_CONTROL, 0x000002, 0x00000703);
		break;
	}

	/* Make sure that we are not swapping colours around on r600+*/
	write32(regOffset + D1GRPH_SWAP_CNTL, 0);

	uint32 fbIntAddress = read32(R6XX_CONFIG_FB_BASE);

	uint32 offset = gInfo->shared_info->frame_buffer_offset;
	write32(regOffset + D1GRPH_PRIMARY_SURFACE_ADDRESS,
		fbIntAddress + offset);
	write32(regOffset + D1GRPH_PITCH, bytesPerRow / 4);
	write32(regOffset + D1GRPH_SURFACE_OFFSET_X, 0);
	write32(regOffset + D1GRPH_SURFACE_OFFSET_Y, 0);
	write32(regOffset + D1GRPH_X_START, 0);
	write32(regOffset + D1GRPH_Y_START, 0);
	write32(regOffset + D1GRPH_X_END, mode->virtual_width);
	write32(regOffset + D1GRPH_Y_END, mode->virtual_height);

	/* D1Mode registers */
	write32(regOffset + D1MODE_DESKTOP_HEIGHT, mode->virtual_height);

	// update shared info
	gInfo->shared_info->bytes_per_row = bytesPerRow;
	gInfo->shared_info->current_mode = *mode;
	gInfo->shared_info->bits_per_pixel = bitsPerPixel;

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

