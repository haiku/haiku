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
CardBlankSet(uint8 crtid, bool blank)
{
	int blackColorReg
		= crtid == 1 ? D2CRTC_BLACK_COLOR : D1CRTC_BLACK_COLOR;
	int blankControlReg
		= crtid == 1 ? D2CRTC_BLANK_CONTROL : D1CRTC_BLANK_CONTROL;

	Write32(CRT, blackColorReg, 0);
	Write32Mask(CRT, blankControlReg, blank ? 1 << 8 : 0, 1 << 8);
}


static void
CardFBSet(uint8 crtid, display_mode *mode)
{
	register_info* regs = gDisplay[crtid]->regs;

	uint32 colorMode;
	uint32 bytesPerRow;
	uint32 bitsPerPixel;

	get_color_space_format(*mode, colorMode, bytesPerRow, bitsPerPixel);

	LVDSAllIdle();
		// DVI / HDMI / LCD
	TMDSAllIdle();
		// DVI / HDMI
	DACAllIdle();
		// VGA

	// framebuffersize = w * h * bpp  =  fb bits / 8 = bytes needed
	//uint64 fbAddress = gInfo->shared_info->frame_buffer_phys;
	uint64 fbAddressInt = gInfo->shared_info->frame_buffer_int;

	// Set the inital frame buffer location in the memory controler
	uint32 mcFbSize;
	MCFBLocation(fbAddressInt, &mcFbSize);
	//MCFBSetup(gInfo->shared_info->frame_buffer_int, mcFbSize);

	Write32(CRT, regs->grphUpdate, (1<<16));
		// Lock for update (isn't this normally the other way around on VGA?

	// Tell GPU which frame buffer address to draw from
	Write32(CRT, regs->grphPrimarySurfaceAddr, fbAddressInt & 0xffffffff);
	Write32(CRT, regs->grphSecondarySurfaceAddr, fbAddressInt & 0xffffffff);

	if (gInfo->shared_info->device_chipset >= (RADEON_R700 | 0x70)) {
		Write32(CRT, regs->grphPrimarySurfaceAddrHigh,
			(fbAddressInt >> 32) & 0xf);
		Write32(CRT, regs->grphSecondarySurfaceAddrHigh,
			(fbAddressInt >> 32) & 0xf);
	}

	Write32(CRT, regs->grphControl, 0);
		// Reset stored depth, format, etc

	// set color mode on video card
	switch (mode->space) {
		case B_CMAP8:
			Write32Mask(CRT, regs->grphControl,
				0, 0x00000703);
			break;
		case B_RGB15_LITTLE:
			Write32Mask(CRT, regs->grphControl,
				0x000001, 0x00000703);
			break;
		case B_RGB16_LITTLE:
			Write32Mask(CRT, regs->grphControl,
				0x000101, 0x00000703);
			break;
		case B_RGB24_LITTLE:
		case B_RGB32_LITTLE:
		default:
			Write32Mask(CRT, regs->grphControl,
				0x000002, 0x00000703);
			break;
	}

	Write32(CRT, regs->grphSwapControl, 0);
		// only for chipsets > r600
		// R5xx - RS690 case is GRPH_CONTROL bit 16

	Write32Mask(CRT, regs->grphEnable, 1, 0x00000001);
		// Enable graphics

	Write32(CRT, regs->grphSurfaceOffsetX, 0);
	Write32(CRT, regs->grphSurfaceOffsetY, 0);
	Write32(CRT, regs->grphXStart, 0);
	Write32(CRT, regs->grphYStart, 0);
	Write32(CRT, regs->grphXEnd, mode->virtual_width);
	Write32(CRT, regs->grphYEnd, mode->virtual_height);
	Write32(CRT, regs->grphPitch, bytesPerRow / 4);

	Write32(CRT, regs->modeDesktopHeight, mode->virtual_height);

	Write32(CRT, regs->grphUpdate, 0);
		// Unlock changed registers

	// update shared info
	gInfo->shared_info->bytes_per_row = bytesPerRow;
	gInfo->shared_info->current_mode = *mode;
	gInfo->shared_info->bits_per_pixel = bitsPerPixel;
}


static void
CardModeSet(uint8 crtid, display_mode *mode)
{
	display_timing& displayTiming = mode->timing;
	register_info* regs = gDisplay[crtid]->regs;

	TRACE("%s called to do %dx%d\n",
		__func__, displayTiming.h_display, displayTiming.v_display);

	// enable read requests
	Write32Mask(CRT, regs->grphControl, 0, 0x01000000);

	// *** Horizontal
	Write32(CRT, regs->crtHTotal,
		displayTiming.h_total - 1);

	// Blanking
	uint16 blankStart = displayTiming.h_total - displayTiming.h_sync_start;
	uint16 blankEnd = displayTiming.h_total
		+ displayTiming.h_display - displayTiming.h_sync_start;

	Write32(CRT, regs->crtHBlank,
		blankStart | (blankEnd << 16));

	Write32(CRT, regs->crtHSync,
		(displayTiming.h_sync_end - displayTiming.h_sync_start) << 16);

	// set flag for neg. H sync. M76 Register Reference Guide 2-256
	Write32Mask(CRT, regs->crtHPolarity,
		displayTiming.flags & B_POSITIVE_HSYNC ? 0 : 1, 0x1);

	// *** Vertical
	Write32(CRT, regs->crtVTotal,
		displayTiming.v_total - 1);

	// Blanking
	blankStart = displayTiming.v_total - displayTiming.v_sync_start;
	blankEnd = displayTiming.v_total
		+ displayTiming.v_display - displayTiming.v_sync_start;

	Write32(CRT, regs->crtVBlank,
		blankStart | (blankEnd << 16));

	// Set Interlace if specified within mode line
	if (displayTiming.flags & B_TIMING_INTERLACED) {
		Write32(CRT, regs->crtInterlace, 0x1);
		Write32(CRT, regs->modeDataFormat, 0x1);
	} else {
		Write32(CRT, regs->crtInterlace, 0x0);
		Write32(CRT, regs->modeDataFormat, 0x0);
	}

	Write32(CRT, regs->crtVSync,
		(displayTiming.v_sync_end - displayTiming.v_sync_start) << 16);

	// set flag for neg. V sync. M76 Register Reference Guide 2-258
	Write32Mask(CRT, regs->crtVPolarity,
		displayTiming.flags & B_POSITIVE_VSYNC ? 0 : 1, 0x1);

	/*	set D1CRTC_HORZ_COUNT_BY2_EN to 0;
		should only be set to 1 on 30bpp DVI modes
	*/
	Write32Mask(CRT, regs->crtCountControl, 0x0, 0x1);
}


static void
CardModeScale(uint8 crtid, display_mode *mode)
{
	register_info* regs = gDisplay[crtid]->regs;

	// No scaling
	Write32(CRT, regs->sclUpdate, (1<<16));// Lock

	#if 0
	Write32(CRT, D1MODE_EXT_OVERSCAN_LEFT_RIGHT,
		(OVERSCAN << 16) | OVERSCAN); // LEFT | RIGHT
	Write32(CRT, D1MODE_EXT_OVERSCAN_TOP_BOTTOM,
		(OVERSCAN << 16) | OVERSCAN); // TOP | BOTTOM
	#endif

	Write32(CRT, regs->viewportStart, 0);
	Write32(CRT, regs->viewportSize,
		mode->timing.v_display | (mode->timing.h_display << 16));
	Write32(CRT, regs->sclEnable, 0);
	Write32(CRT, regs->sclTapControl, 0);
	Write32(CRT, regs->modeCenter, 2);
	// D1MODE_DATA_FORMAT?
	Write32(CRT, regs->sclUpdate, 0);		// Unlock
}


status_t
radeon_set_display_mode(display_mode *mode)
{
	uint8 display_id = 0;

	CardFBSet(display_id, mode);
	CardModeSet(display_id, mode);
	CardModeScale(display_id, mode);

	// If this is DAC, set our PLL
	if ((gDisplay[display_id]->connection_type & CONNECTION_DAC) != 0) {
		PLLSet(gDisplay[display_id]->connection_id, mode->timing.pixel_clock);
		DACSet(gDisplay[display_id]->connection_id, display_id);

		// TODO : Shutdown unused PLL/DAC

		// Power up the output
		PLLPower(gDisplay[display_id]->connection_id, RHD_POWER_ON);
		DACPower(gDisplay[display_id]->connection_id, RHD_POWER_ON);
	} else if ((gDisplay[display_id]->connection_type & CONNECTION_TMDS) != 0) {
		TMDSSet(gDisplay[display_id]->connection_id, mode);
		TMDSPower(gDisplay[display_id]->connection_id, RHD_POWER_ON);
	} else if ((gDisplay[display_id]->connection_type & CONNECTION_LVDS) != 0) {
		LVDSSet(gDisplay[display_id]->connection_id, mode);
		LVDSPower(gDisplay[display_id]->connection_id, RHD_POWER_ON);
	}

	// Ensure screen isn't blanked
	CardBlankSet(display_id, false);

	int32 crtstatus = Read32(CRT, D1CRTC_STATUS);
	TRACE("CRT0 Status: 0x%X\n", crtstatus);
	crtstatus = Read32(CRT, D2CRTC_STATUS);
	TRACE("CRT1 Status: 0x%X\n", crtstatus);

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

	//*_low = 48L;
	//*_high = 100 * 1000000L;
	return B_OK;
}


bool
is_mode_supported(display_mode *mode)
{
	TRACE("MODE: %d ; %d %d %d %d ; %d %d %d %d\n",
		mode->timing.pixel_clock, mode->timing.h_display,
		mode->timing.h_sync_start, mode->timing.h_sync_end,
		mode->timing.h_total, mode->timing.v_display,
		mode->timing.v_sync_start, mode->timing.v_sync_end,
		mode->timing.v_total);

	// Validate modeline is within a sane range
	if (is_mode_sane(mode) != B_OK)
		return false;

	// TODO : is_mode_supported on *which* display?
	uint32 crtid = 0;

	// if we have edid info, check frequency adginst crt reported valid ranges
	if (gInfo->shared_info->has_edid
		&& gDisplay[crtid]->found_ranges) {

		uint32 hfreq = mode->timing.pixel_clock / mode->timing.h_total;
		if (hfreq > gDisplay[crtid]->hfreq_max + 1
			|| hfreq < gDisplay[crtid]->hfreq_min - 1) {
			TRACE("!!! hfreq : %d , hfreq_min : %d, hfreq_max : %d\n",
				hfreq, gDisplay[crtid]->hfreq_min, gDisplay[crtid]->hfreq_max);
			TRACE("!!! %dx%d falls outside of CRT %d's valid "
				"horizontal range.\n", mode->timing.h_display,
				mode->timing.v_display, crtid);
			return false;
		}

		uint32 vfreq = mode->timing.pixel_clock / ((mode->timing.v_total
			* mode->timing.h_total) / 1000);

		if (vfreq > gDisplay[crtid]->vfreq_max + 1
			|| vfreq < gDisplay[crtid]->vfreq_min - 1) {
			TRACE("!!! vfreq : %d , vfreq_min : %d, vfreq_max : %d\n",
				vfreq, gDisplay[crtid]->vfreq_min, gDisplay[crtid]->vfreq_max);
			TRACE("!!! %dx%d falls outside of CRT %d's valid vertical range\n",
				mode->timing.h_display, mode->timing.v_display, crtid);
			return false;
		}
	}

	TRACE("%dx%d is within CRT %d's valid frequency range\n",
		mode->timing.h_display, mode->timing.v_display, crtid);

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


