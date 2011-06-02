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
}


// Blacks the screen out, useful for mode setting
static void
CardBlankSet(bool blank)
{
	int blackColorReg;
	int blankControlReg;

	blackColorReg = D1CRTC_BLACK_COLOR;
	blankControlReg = D1CRTC_BLANK_CONTROL;

	write32(blackColorReg, 0);
	write32AtMask(blankControlReg, blank ? 1 << 8 : 0, 1 << 8);
}


static void
CardFBSet(display_mode *mode)
{
	uint32 colorMode;
	uint32 bytesPerRow;
	uint32 bitsPerPixel;

	get_color_space_format(*mode, colorMode, bytesPerRow, bitsPerPixel);

	// disable R/B swap, disable tiling, disable 16bit alpha, etc.
	write32AtMask(gRegister->grphEnable, 1, 0x00000001);
	write32(gRegister->grphControl, 0);

	// set color mode on video card
	switch (mode->space) {
		case B_CMAP8:
			write32AtMask(gRegister->grphControl,
				0, 0x00000703);
			break;
		case B_RGB15_LITTLE:
			write32AtMask(gRegister->grphControl,
				0x000001, 0x00000703);
			break;
		case B_RGB16_LITTLE:
			write32AtMask(gRegister->grphControl,
				0x000101, 0x00000703);
			break;
		case B_RGB24_LITTLE:
		case B_RGB32_LITTLE:
		default:
			write32AtMask(gRegister->grphControl,
				0x000002, 0x00000703);
			break;
	}

	write32(gRegister->grphSwapControl, 0);
		// only for chipsets > r600
		// R5xx - RS690 case is GRPH_CONTROL bit 16


	// framebuffersize = w * h * bpp  =  fb bits / 8 = bytes needed

	uint64_t fbAddress = gInfo->shared_info->frame_buffer_phys;

	// Tell GPU which frame buffer address to draw from
	if (gInfo->shared_info->device_chipset >= (uint16)(RADEON_R700 & 0x70)) {
		write32(gRegister->grphPrimarySurfaceAddrHigh,
			(fbAddress >> 32) & 0xf);
		write32(gRegister->grphSecondarySurfaceAddrHigh,
			(fbAddress >> 32) & 0xf);
	}

	write32(gRegister->grphPrimarySurfaceAddr,
		fbAddress & 0xffffffff);
	write32(gRegister->grphSecondarySurfaceAddr,
		fbAddress & 0xffffffff);

	write32(gRegister->grphPitch, bytesPerRow / 4);
	write32(gRegister->grphSurfaceOffsetX, 0);
	write32(gRegister->grphSurfaceOffsetY, 0);
	write32(gRegister->grphXStart, 0);
	write32(gRegister->grphYStart, 0);
	write32(gRegister->grphXEnd, mode->virtual_width);
	write32(gRegister->grphYEnd, mode->virtual_height);

	/* D1Mode registers */
	write32(gRegister->modeDesktopHeight, mode->virtual_height);

	// update shared info
	gInfo->shared_info->bytes_per_row = bytesPerRow;
	gInfo->shared_info->current_mode = *mode;
	gInfo->shared_info->bits_per_pixel = bitsPerPixel;
}


static void
CardModeSet(display_mode *mode)
{
	CardBlankSet(true);

	display_timing& displayTiming = mode->timing;

	TRACE("%s called to do %dx%d\n",
		__func__, displayTiming.h_display, displayTiming.v_display);

	// enable read requests
	write32AtMask(gRegister->grphControl, 0, 0x01000000);

	// *** Horizontal
	write32(gRegister->crtHTotal,
		displayTiming.h_total - 1);

	// determine blanking based on passed modeline
	//uint16 blankStart = displayTiming.h_display + 1;
	//uint16 blankEnd = displayTiming.h_total - 1;

	//write32(gRegister->crtHBlank,
	//	blankStart | (blankEnd << 16));

	write32(gRegister->crtHSync,
		(displayTiming.h_sync_end - displayTiming.h_sync_start) << 16);

	// set flag for neg. H sync. M76 Register Reference Guide 2-256
	write32AtMask(gRegister->crtHPolarity,
		displayTiming.flags & B_POSITIVE_HSYNC ? 0 : 1, 0x1);

	// *** Vertical
	write32(gRegister->crtVTotal,
		displayTiming.v_total - 1);

	//blankStart = displayTiming.v_display + 1;
	//blankEnd = displayTiming.v_total - 1;

	//write32(gRegister->crtVBlank,
	//	blankStart | (blankEnd << 16));

	// Set Interlace if specified within mode line
	if (displayTiming.flags & B_TIMING_INTERLACED) {
		write32(gRegister->crtInterlace, 0x1);
		write32(gRegister->modeDataFormat, 0x1);
	} else {
		write32(gRegister->crtInterlace, 0x0);
		write32(gRegister->modeDataFormat, 0x0);
	}

	write32(gRegister->crtVSync,
		(displayTiming.v_sync_end - displayTiming.v_sync_start) << 16);

	// set flag for neg. V sync. M76 Register Reference Guide 2-258
	// we don't need a mask here as this is the only param for Vertical
	write32(gRegister->crtVPolarity,
		displayTiming.flags & B_POSITIVE_VSYNC ? 0 : 1);

	/*	set D1CRTC_HORZ_COUNT_BY2_EN to 0;
		should only be set to 1 on 30bpp DVI modes
	*/
	write32AtMask(gRegister->crtCountControl, 0x0, 0x1);

	CardBlankSet(false);
}


static void
CardModeScale(display_mode *mode)
{
	write32(gRegister->viewportSize,
		mode->timing.v_display | (mode->timing.h_display << 16));
	write32(gRegister->viewportStart, 0);

	// For now, no overscan support
	write32(D1MODE_EXT_OVERSCAN_LEFT_RIGHT,
		(0 << 16) | 0);
	write32(D1MODE_EXT_OVERSCAN_TOP_BOTTOM,
		(0 << 16) | 0);

/*  write32(regOffset + D1MODE_EXT_OVERSCAN_LEFT_RIGHT,
		(Overscan.OverscanLeft << 16) | Overscan.OverscanRight);
	write32(regOffset + D1MODE_EXT_OVERSCAN_TOP_BOTTOM,
		(Overscan.OverscanTop << 16) | Overscan.OverscanBottom);
*/

	// No scaling
	write32(gRegister->sclUpdate, (1<<16));	// Lock
	write32(gRegister->sclEnable, 0);
	write32(gRegister->sclTapControl, 0);
	write32(gRegister->modeCenter, 0);
	write32(gRegister->sclUpdate, 0);		// Unlock

	#if 0
	// Auto scale keeping aspect ratio
	write32(regOffset + D1MODE_CENTER, 1);

	write32(regOffset + D1SCL_UPDATE, 0);
	write32(regOffset + D1SCL_FLIP_CONTROL, 0);

	write32(regOffset + D1SCL_ENABLE, 1);
	write32(regOffset + D1SCL_HVSCALE, 0x00010001);

	write32(regOffset + D1SCL_TAP_CONTROL, 0x00000101);

	write32(regOffset + D1SCL_HFILTER, 0x00030100);
	write32(regOffset + D1SCL_VFILTER, 0x00030100);

	write32(regOffset + D1SCL_DITHER, 0x00001010);
	#endif
}


status_t
radeon_set_display_mode(display_mode *mode)
{
	int crtNumber = 0;

	init_registers(crtNumber);
	CardFBSet(mode);
	CardModeSet(mode);
	CardModeScale(mode);

	int32 crtstatus = read32(D1CRTC_STATUS);
	TRACE("CRT0 Status: 0x%X\n", crtstatus);

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

	config->frame_buffer = gInfo->shared_info->frame_buffer;
	config->frame_buffer_dma = (uint8 *)gInfo->shared_info->frame_buffer_phys;

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
		uint32 totalClocks = (uint32)mode->timing.h_total
			*(uint32)mode->timing.v_total;
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
	// Validate modeline is within a sane range
	if (is_mode_sane(mode) != B_OK)
		return false;

	// TODO : Look at min and max monitor freqs and verify selected
	// mode is within tolerances.
	#if 0
	int crtid = 0;

	edid1_detailed_monitor *monitor
		= &gInfo->shared_info->edid_info.detailed_monitor[crtid + 1];
	edid1_monitor_range& range = monitor->data.monitor_range;

	TRACE("%s CRT Min/Max H %d/%d; CRT Min/Max V %d/%d\n", __func__,
		range.min_h, range.max_h, range.min_v, range.max_v);
	#endif

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

