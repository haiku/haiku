/*
 * Copyright 2006-2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Support for i915 chipset and up based on the X driver,
 * Copyright 2006-2007 Intel Corporation.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Alexander von Gluck, kallisti5@unixzen.com
 */


#include "mode.h"

#include <create_display_modes.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "accelerant.h"
#include "accelerant_protos.h"
#include "bios.h"
#include "connector.h"
#include "display.h"
#include "displayport.h"
#include "encoder.h"
#include "pll.h"
#include "utility.h"


#define TRACE_MODE
#ifdef TRACE_MODE
extern "C" void _sPrintf(const char* format, ...);
#	define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#	define TRACE(x...) ;
#endif

#define ERROR(x...) _sPrintf("radeon_hd: " x)


status_t
create_mode_list(void)
{
	// TODO: multi-monitor?  for now we use VESA and not gDisplay edid
	uint8 crtcID = 0;

	const color_space kRadeonHDSpaces[] = {B_RGB32_LITTLE, B_RGB24_LITTLE,
		B_RGB16_LITTLE, B_RGB15_LITTLE, B_CMAP8};

	gInfo->mode_list_area = create_display_modes("radeon HD modes",
		&gDisplay[crtcID]->edidData, NULL, 0, kRadeonHDSpaces,
		B_COUNT_OF(kRadeonHDSpaces), is_mode_supported, &gInfo->mode_list,
		&gInfo->shared_info->mode_count);
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
	// TODO: multi-monitor?  we need crtcid here

	return gInfo->shared_info->mode_count;
}


status_t
radeon_get_mode_list(display_mode* modeList)
{
	TRACE("%s\n", __func__);
	// TODO: multi-monitor?  we need crtcid here
	memcpy(modeList, gInfo->mode_list,
		gInfo->shared_info->mode_count * sizeof(display_mode));
	return B_OK;
}


status_t
radeon_get_preferred_mode(display_mode* preferredMode)
{
	TRACE("%s\n", __func__);
	// TODO: multi-monitor?  we need crtcid here

	uint8_t crtc = 0;

	if (gDisplay[crtc]->preferredMode.virtual_width > 0
		&& gDisplay[crtc]->preferredMode.virtual_height > 0) {
		TRACE("%s: preferred mode was found for display %" B_PRIu8 "\n",
			__func__, crtc);
		memcpy(preferredMode, &gDisplay[crtc]->preferredMode,
			sizeof(gDisplay[crtc]->preferredMode));
		return B_OK;
	}

	return B_ERROR;
}


status_t
radeon_get_edid_info(void* info, size_t size, uint32* edid_version)
{
	// TODO: multi-monitor?  for now we use display 0
	uint8 crtcID = 0;

	TRACE("%s\n", __func__);
	if (!gInfo->shared_info->has_edid)
		return B_ERROR;
	if (size < sizeof(struct edid1_info))
		return B_BUFFER_OVERFLOW;

	//memcpy(info, &gInfo->shared_info->edid_info, sizeof(struct edid1_info));
		// VESA
	memcpy(info, &gDisplay[crtcID]->edidData, sizeof(struct edid1_info));
		// Display 0

	*edid_version = EDID_VERSION_1;

	return B_OK;
}


uint32
radeon_dpms_capabilities(void)
{
	// These should be pretty universally supported on Radeon HD cards
	return B_DPMS_ON | B_DPMS_STAND_BY | B_DPMS_SUSPEND | B_DPMS_OFF;
}


uint32
radeon_dpms_mode(void)
{
	// TODO: this really isn't a good long-term solution
	// we may need to look at the encoder dpms scratch registers
	return gInfo->dpms_mode;
}


void
radeon_dpms_set(uint8 id, int mode)
{
	if (mode == B_DPMS_ON) {
		display_crtc_dpms(id, mode);
		encoder_dpms_set(id, mode);
	} else {
		encoder_dpms_set(id, mode);
		display_crtc_dpms(id, mode);
	}
	gInfo->dpms_mode = mode;
}


void
radeon_dpms_set_hook(int mode)
{
	// TODO: multi-monitor? 

	uint8 crtcID = 0;

	if (gDisplay[crtcID]->attached)
		radeon_dpms_set(crtcID, mode);
}


status_t
radeon_set_display_mode(display_mode* mode)
{
	// TODO: multi-monitor? For now we set the mode on
	// the first display found.

	TRACE("%s\n", __func__);
	TRACE("  mode->space: %#" B_PRIx32 "\n", mode->space);
	TRACE("  mode->virtual_width: %" B_PRIu16 "\n", mode->virtual_width);
	TRACE("  mode->virtual_height: %" B_PRIu16 "\n", mode->virtual_height);
	TRACE("  mode->h_display_start: %" B_PRIu16 "\n", mode->h_display_start);
	TRACE("  mode->v_display_start: %" B_PRIu16 "\n", mode->v_display_start);
	TRACE("  mode->flags: %#" B_PRIx32 "\n", mode->flags);

	uint8 crtcID = 0;

	if (gDisplay[crtcID]->attached == false)
		return B_ERROR;

	// Copy this display mode into the "current mode" for the display
	memcpy(&gDisplay[crtcID]->currentMode, mode, sizeof(display_mode));

	uint32 connectorIndex = gDisplay[crtcID]->connectorIndex;

	// Determine DP lanes if DP
	if (connector_is_dp(connectorIndex)) {
		dp_info *dpInfo = &gConnector[connectorIndex]->dpInfo;
		dpInfo->laneCount = dp_get_lane_count(connectorIndex, mode);
		dpInfo->linkRate = dp_get_link_rate(connectorIndex, mode);
	}

	// *** crtc and encoder prep
	encoder_output_lock(true);
	display_crtc_lock(crtcID, ATOM_ENABLE);
	radeon_dpms_set(crtcID, B_DPMS_OFF);

	// *** Set up encoder -> crtc routing
	encoder_assign_crtc(crtcID);

	// *** CRT controler mode set
	// Set up PLL for connector
	pll_pick(connectorIndex);
	pll_info* pll = &gConnector[connectorIndex]->encoder.pll;
	TRACE("%s: pll %d selected for connector %" B_PRIu32 "\n", __func__,
		pll->id, connectorIndex);
	pll_set(mode, crtcID);

	display_crtc_set_dtd(crtcID, mode);

	display_crtc_fb_set(crtcID, mode);
	// atombios_overscan_setup
	display_crtc_scale(crtcID, mode);

	// *** encoder mode set
	encoder_mode_set(crtcID);

	// *** encoder and CRT controller commit
	radeon_dpms_set(crtcID, B_DPMS_ON);
	display_crtc_lock(crtcID, ATOM_DISABLE);
	encoder_output_lock(false);

	#ifdef TRACE_MODE
	// for debugging
	debug_dp_info();

	TRACE("D1CRTC_STATUS        Value: 0x%X\n",
		Read32(CRT, AVIVO_D1CRTC_STATUS));
	TRACE("D2CRTC_STATUS        Value: 0x%X\n",
		Read32(CRT, AVIVO_D2CRTC_STATUS));
	TRACE("D1CRTC_CONTROL       Value: 0x%X\n",
		Read32(CRT, AVIVO_D1CRTC_CONTROL));
	TRACE("D2CRTC_CONTROL       Value: 0x%X\n",
		Read32(CRT, AVIVO_D2CRTC_CONTROL));
	TRACE("D1GRPH_ENABLE        Value: 0x%X\n",
		Read32(CRT, AVIVO_D1GRPH_ENABLE));
	TRACE("D2GRPH_ENABLE        Value: 0x%X\n",
		Read32(CRT, AVIVO_D2GRPH_ENABLE));
	TRACE("D1SCL_ENABLE         Value: 0x%X\n",
		Read32(CRT, AVIVO_D1SCL_SCALER_ENABLE));
	TRACE("D2SCL_ENABLE         Value: 0x%X\n",
		Read32(CRT, AVIVO_D2SCL_SCALER_ENABLE));
	TRACE("D1CRTC_BLANK_CONTROL Value: 0x%X\n",
		Read32(CRT, AVIVO_D1CRTC_BLANK_CONTROL));
	TRACE("D2CRTC_BLANK_CONTROL Value: 0x%X\n",
		Read32(CRT, AVIVO_D1CRTC_BLANK_CONTROL));
	#endif

	return B_OK;
}


status_t
radeon_get_display_mode(display_mode* _currentMode)
{
	TRACE("%s\n", __func__);

	*_currentMode = gInfo->shared_info->current_mode;
	//*_currentMode = gDisplay[X]->currentMode;
	return B_OK;
}


status_t
radeon_get_frame_buffer_config(frame_buffer_config* config)
{
	TRACE("%s\n", __func__);

	config->frame_buffer = gInfo->shared_info->frame_buffer;
	config->frame_buffer_dma = (uint8*)gInfo->shared_info->frame_buffer_phys;

	config->bytes_per_row = gInfo->shared_info->bytes_per_row;

	TRACE("  config->frame_buffer: %#" B_PRIxADDR "\n",
		(phys_addr_t)config->frame_buffer);
	TRACE("  config->frame_buffer_dma: %#" B_PRIxADDR "\n",
		(phys_addr_t)config->frame_buffer_dma);
	TRACE("  config->bytes_per_row: %" B_PRIu32 "\n", config->bytes_per_row);

	return B_OK;
}


status_t
radeon_get_pixel_clock_limits(display_mode* mode, uint32* _low, uint32* _high)
{
	TRACE("%s\n", __func__);

	if (_low != NULL) {
		// lower limit of about 48Hz vertical refresh
		uint32 totalClocks = (uint32)mode->timing.h_total
			* (uint32)mode->timing.v_total;
		uint32 low = (totalClocks * 48L) / 1000L;

		if (low < PLL_MIN_DEFAULT)
			low = PLL_MIN_DEFAULT;
		else if (low > PLL_MAX_DEFAULT)
			return B_ERROR;

		*_low = low;
	}

	if (_high != NULL)
		*_high = PLL_MAX_DEFAULT;

	//*_low = 48L;
	//*_high = 100 * 1000000L;
	return B_OK;
}


bool
is_mode_supported(display_mode* mode)
{
	bool sane = true;

	// Validate modeline is within a sane range
	if (is_mode_sane(mode) != B_OK)
		sane = false;

	// TODO: is_mode_supported on *which* display?
	uint32 crtid = 0;

	// if we have edid info, check frequency adginst crt reported valid ranges
	if (gInfo->shared_info->has_edid
		&& gDisplay[crtid]->foundRanges) {

		// validate horizontal frequency range
		uint32 hfreq = mode->timing.pixel_clock / mode->timing.h_total;
		if (hfreq > gDisplay[crtid]->hfreqMax + 1
			|| hfreq < gDisplay[crtid]->hfreqMin - 1) {
			//TRACE("!!! mode below falls outside of hfreq range!\n");
			sane = false;
		}

		// validate vertical frequency range
		uint32 vfreq = mode->timing.pixel_clock / ((mode->timing.v_total
			* mode->timing.h_total) / 1000);
		if (vfreq > gDisplay[crtid]->vfreqMax + 1
			|| vfreq < gDisplay[crtid]->vfreqMin - 1) {
			//TRACE("!!! mode below falls outside of vfreq range!\n");
			sane = false;
		}
	}

	#if 0
	// Lots of spam, but good for understanding what modelines are in use
	TRACE("MODE: %d ; %d %d %d %d ; %d %d %d %d is %s\n",
		mode->timing.pixel_clock, mode->timing.h_display,
		mode->timing.h_sync_start, mode->timing.h_sync_end,
		mode->timing.h_total, mode->timing.v_display,
		mode->timing.v_sync_start, mode->timing.v_sync_end,
		mode->timing.v_total,
		sane ? "OK." : "BAD, out of range!");
	#endif

	return sane;
}


/*
 * A quick sanity check of the provided display_mode
 */
status_t
is_mode_sane(display_mode* mode)
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


uint32
get_mode_bpp(display_mode* mode)
{
	// Get bitsPerPixel for given mode

	switch (mode->space) {
		case B_CMAP8:
			return 8;
		case B_RGB15_LITTLE:
			return 15;
		case B_RGB16_LITTLE:
			return 16;
		case B_RGB24_LITTLE:
		case B_RGB32_LITTLE:
			return 32;
	}
	ERROR("%s: Unknown colorspace for mode, guessing 32 bits per pixel\n",
		__func__);
	return 32;
}
