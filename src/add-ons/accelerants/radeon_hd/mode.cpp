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


#define TRACE_MODE
#ifdef TRACE_MODE
extern "C" void _sPrintf(const char *format, ...);
#	define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#	define TRACE(x...) ;
#endif


static display_mode *gDisplayMode;


status_t
create_mode_list(void)
{
	// TODO : Read active monitor EDID

	/* Populate modeline with temporary example */
	gDisplayMode->timing.pixel_clock = 71500;
	gDisplayMode->timing.h_display = 1366;	// In Pixels
	gDisplayMode->timing.h_sync_start = 1406;
	gDisplayMode->timing.h_sync_end = 1438;
	gDisplayMode->timing.h_total = 1510;
	gDisplayMode->timing.v_display = 768;	// In Pixels
	gDisplayMode->timing.v_sync_start = 771;
	gDisplayMode->timing.v_sync_end = 777;
	gDisplayMode->timing.v_total = 789;
	gDisplayMode->timing.flags = 0;			// Polarity, ex: B_POSITIVE_HSYNC

	gDisplayMode->space = B_RGB32_LITTLE;	// Pixel configuration
	gDisplayMode->virtual_width = 1366;		// In Pixels
	gDisplayMode->virtual_height = 768;		// In Pixels
	gDisplayMode->h_display_start = 0;
	gDisplayMode->v_display_start = 0;
	gDisplayMode->flags = 0;				// Mode flags (Some drivers use this

	mode_sanity_check(gDisplayMode);

	gInfo->mode_list = gDisplayMode;
	gInfo->shared_info->mode_count = 1;
	return B_OK;
}


//	#pragma mark -


uint32
radeon_accelerant_mode_count(void)
{
	TRACE("%d\n", __func__);

	return gInfo->shared_info->mode_count;
}


status_t
radeon_get_mode_list(display_mode *modeList)
{
	TRACE("%d\n", __func__);
	memcpy(modeList, gInfo->mode_list,
		gInfo->shared_info->mode_count * sizeof(display_mode));
	return B_OK;
}


inline void
write32AtMask(uint32 adress, uint32 value, uint32 mask)
{
	uint32 temp;
	temp = read32(adress);
	temp &= ~mask;
	temp |= value & mask;
	write32(adress, temp);
}


enum {
	/* CRTC1 registers */
	D1CRTC_H_TOTAL                 = 0x6000,
	D1CRTC_H_BLANK_START_END       = 0x6004,
	D1CRTC_H_SYNC_A                = 0x6008,
	D1CRTC_H_SYNC_A_CNTL           = 0x600C,
	D1CRTC_H_SYNC_B                = 0x6010,
	D1CRTC_H_SYNC_B_CNTL           = 0x6014,

	D1CRTC_V_TOTAL                 = 0x6020,
	D1CRTC_V_BLANK_START_END       = 0x6024,
	D1CRTC_V_SYNC_A                = 0x6028,
	D1CRTC_V_SYNC_A_CNTL           = 0x602C,
	D1CRTC_V_SYNC_B                = 0x6030,
	D1CRTC_V_SYNC_B_CNTL           = 0x6034,

	D1CRTC_CONTROL                 = 0x6080,
	D1CRTC_BLANK_CONTROL           = 0x6084,
	D1CRTC_INTERLACE_CONTROL	   = 0x6088,
	D1CRTC_BLACK_COLOR             = 0x6098,
	D1CRTC_STATUS                  = 0x609C,
	D1CRTC_COUNT_CONTROL           = 0x60B4,

	/* D1GRPH registers */
	D1GRPH_ENABLE                  = 0x6100,
	D1GRPH_CONTROL                 = 0x6104,
	D1GRPH_LUT_SEL                 = 0x6108,
	D1GRPH_SWAP_CNTL               = 0x610C,
	D1GRPH_PRIMARY_SURFACE_ADDRESS = 0x6110,
	D1GRPH_SECONDARY_SURFACE_ADDRESS = 0x6118,
	D1GRPH_PITCH                   = 0x6120,
	D1GRPH_SURFACE_OFFSET_X        = 0x6124,
	D1GRPH_SURFACE_OFFSET_Y        = 0x6128,
	D1GRPH_X_START                 = 0x612C,
	D1GRPH_Y_START                 = 0x6130,
	D1GRPH_X_END                   = 0x6134,
	D1GRPH_Y_END                   = 0x6138,
	D1GRPH_UPDATE                  = 0x6144,

	/* D1MODE */
	D1MODE_DESKTOP_HEIGHT          = 0x652C,
	D1MODE_VLINE_START_END         = 0x6538,
	D1MODE_VLINE_STATUS            = 0x653C,
	D1MODE_VIEWPORT_START          = 0x6580,
	D1MODE_VIEWPORT_SIZE           = 0x6584,
	D1MODE_EXT_OVERSCAN_LEFT_RIGHT = 0x6588,
	D1MODE_EXT_OVERSCAN_TOP_BOTTOM = 0x658C,
	D1MODE_DATA_FORMAT             = 0x6528,

	/* D1SCL */
	D1SCL_ENABLE                   = 0x6590,
	D1SCL_TAP_CONTROL              = 0x6594,
	D1MODE_CENTER                  = 0x659C, /* guess */
	D1SCL_HVSCALE                  = 0x65A4, /* guess */
	D1SCL_HFILTER                  = 0x65B0, /* guess */
	D1SCL_VFILTER                  = 0x65C0, /* guess */
	D1SCL_UPDATE                   = 0x65CC,
	D1SCL_DITHER                   = 0x65D4, /* guess */
	D1SCL_FLIP_CONTROL             = 0x65D8 /* guess */

};


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


#define D1_REG_OFFSET 0x0000
#define D2_REG_OFFSET 0x0800


static void
DxModeSet(display_mode *mode)
{
	uint32 regOffset = D1_REG_OFFSET;

	display_timing& displayTiming = mode->timing;


	/* enable read requests */
	write32AtMask(regOffset + D1CRTC_CONTROL, 0, 0x01000000);

	/* Horizontal */
	write32(regOffset + D1CRTC_H_TOTAL, displayTiming.h_total - 1);

	uint16 blankStart = displayTiming.h_display;	// displayTiming.h_sync_end;
	uint16 blankEnd = displayTiming.h_sync_start;	// displayTiming.h_total;
//	write32(regOffset + D1CRTC_H_BLANK_START_END,
//		blankStart | (blankEnd << 16));

	write32(regOffset + D1CRTC_H_SYNC_A,
		(displayTiming.h_sync_end - displayTiming.h_sync_start) << 16);
//	write32(regOffset + D1CRTC_H_SYNC_A_CNTL, Mode->Flags & V_NHSYNC);
//!	write32(regOffset + D1CRTC_H_SYNC_A_CNTL, V_NHSYNC);

	/* Vertical */
	write32(regOffset + D1CRTC_V_TOTAL, displayTiming.v_total - 1);

	blankStart = displayTiming.v_display;	// displayTiming.v_sync_end;
	blankEnd = displayTiming.v_sync_start;	// displayTiming.v_total;
//  write32(regOffset + D1CRTC_V_BLANK_START_END,
//		blankStart | (blankEnd << 16));

	/* set interlaced */
//	if (Mode->Flags & V_INTERLACE) {
	if (0) {
		write32(regOffset + D1CRTC_INTERLACE_CONTROL, 0x1);
		write32(regOffset + D1MODE_DATA_FORMAT, 0x1);
	} else {
		write32(regOffset + D1CRTC_INTERLACE_CONTROL, 0x0);
		write32(regOffset + D1MODE_DATA_FORMAT, 0x0);
	}

	write32(regOffset + D1CRTC_V_SYNC_A,
		(displayTiming.v_sync_end - displayTiming.v_sync_start) << 16);
//	write32(regOffset + D1CRTC_V_SYNC_A_CNTL, Mode->Flags & V_NVSYNC);
//!	write32(regOffset + D1CRTC_V_SYNC_A_CNTL, V_NVSYNC);

	/*	set D1CRTC_HORZ_COUNT_BY2_EN to 0;
		should only be set to 1 on 30bpp DVI modes
	*/
	write32AtMask(regOffset + D1CRTC_COUNT_CONTROL, 0x0, 0x1);

}


static void
DxModeScale(display_mode *mode)
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
	write32(regOffset + D1MODE_CENTER, 0);
}


status_t
radeon_set_display_mode(display_mode *mode)
{
	DxModeSet(mode);

	DxModeScale(mode);

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
	/* TODO: 64bpp ;p */
	}

	/* Make sure that we are not swapping colours around */
//	if (rhdPtr->ChipSet > RHD_R600)
	write32(regOffset + D1GRPH_SWAP_CNTL, 0);
	/* R5xx - RS690 case is GRPH_CONTROL bit 16 */

#define R6XX_CONFIG_FB_BASE 0x542C /* AKA CONFIG_F0_BASE */

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

	_currentMode = gDisplayMode;
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


/*
 * A quick sanity check of the provided display_mode
 */
status_t
mode_sanity_check(display_mode *mode)
{
	if (mode->timing.h_display <= 0
		|| mode->timing.h_sync_start <= 0
		|| mode->timing.h_sync_end <= 0
		|| mode->timing.h_total <= 0) {
		TRACE("Invalid horizontal mode timing received for %dx%d\n",
			mode->timing.h_display, mode->timing.v_display);
		return B_ERROR;
	}

	if (mode->timing.v_display <= 0
		|| mode->timing.v_sync_start <= 0
		|| mode->timing.v_sync_end <= 0
		|| mode->timing.v_total <= 0) {
		TRACE("Invalid vertical mode timing received for %dx%d\n",
			mode->timing.h_display, mode->timing.v_display);
		return B_ERROR;
	}

	if (mode->flags <= 0) {
		TRACE("Invalid mode timing flag received for %dx%d\n",
			mode->timing.h_display, mode->timing.v_display);
		return B_ERROR;
	}

	return B_OK;
}

