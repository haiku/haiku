/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */


#include "accelerant_protos.h"
#include "accelerant.h"
#include "display.h"

#include <stdlib.h>
#include <string.h>


#define TRACE_DISPLAY
#ifdef TRACE_DISPLAY
extern "C" void _sPrintf(const char *format, ...);
#   define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#   define TRACE(x...) ;
#endif


/*! Populate regs with device dependant register locations */
status_t
init_registers(register_info* regs, uint8 crtid)
{
	memset(regs, 0, sizeof(register_info));

	radeon_shared_info &info = *gInfo->shared_info;

	if (info.device_chipset >= RADEON_R800) {
		uint32 offset = 0;

		// AMD Eyefinity on Evergreen GPUs
		if (crtid == 1) {
			offset = EVERGREEN_CRTC1_REGISTER_OFFSET;
			regs->vgaControl = D2VGA_CONTROL;
		} else if (crtid == 2) {
			offset = EVERGREEN_CRTC2_REGISTER_OFFSET;
			regs->vgaControl = EVERGREEN_D3VGA_CONTROL;
		} else if (crtid == 3) {
			offset = EVERGREEN_CRTC3_REGISTER_OFFSET;
			regs->vgaControl = EVERGREEN_D4VGA_CONTROL;
		} else if (crtid == 4) {
			offset = EVERGREEN_CRTC4_REGISTER_OFFSET;
			regs->vgaControl = EVERGREEN_D5VGA_CONTROL;
		} else if (crtid == 5) {
			offset = EVERGREEN_CRTC5_REGISTER_OFFSET;
			regs->vgaControl = EVERGREEN_D6VGA_CONTROL;
		} else {
			offset = EVERGREEN_CRTC0_REGISTER_OFFSET;
			regs->vgaControl = D1VGA_CONTROL;
		}

		// Evergreen+ is crtoffset + register
		regs->grphEnable = offset + EVERGREEN_GRPH_ENABLE;
		regs->grphControl = offset + EVERGREEN_GRPH_CONTROL;
		regs->grphSwapControl = offset + EVERGREEN_GRPH_SWAP_CONTROL;
		regs->grphPrimarySurfaceAddr
			= offset + EVERGREEN_GRPH_PRIMARY_SURFACE_ADDRESS;
		regs->grphSecondarySurfaceAddr
			= offset + EVERGREEN_GRPH_SECONDARY_SURFACE_ADDRESS;

		regs->grphPrimarySurfaceAddrHigh
			= offset + EVERGREEN_GRPH_PRIMARY_SURFACE_ADDRESS_HIGH;
		regs->grphSecondarySurfaceAddrHigh
			= offset + EVERGREEN_GRPH_SECONDARY_SURFACE_ADDRESS_HIGH;

		regs->grphPitch = offset + EVERGREEN_GRPH_PITCH;
		regs->grphSurfaceOffsetX
			= offset + EVERGREEN_GRPH_SURFACE_OFFSET_X;
		regs->grphSurfaceOffsetY
			= offset + EVERGREEN_GRPH_SURFACE_OFFSET_Y;
		regs->grphXStart = offset + EVERGREEN_GRPH_X_START;
		regs->grphYStart = offset + EVERGREEN_GRPH_Y_START;
		regs->grphXEnd = offset + EVERGREEN_GRPH_X_END;
		regs->grphYEnd = offset + EVERGREEN_GRPH_Y_END;
		regs->crtControl = offset + EVERGREEN_CRTC_CONTROL;
		regs->modeDesktopHeight = offset + EVERGREEN_DESKTOP_HEIGHT;
		regs->modeDataFormat = offset + EVERGREEN_DATA_FORMAT;
		regs->viewportStart = offset + EVERGREEN_VIEWPORT_START;
		regs->viewportSize = offset + EVERGREEN_VIEWPORT_SIZE;

	} else if (info.device_chipset >= RADEON_R600
		&& info.device_chipset < RADEON_R800) {

		// r600 - r700 are D1 or D2 based on primary / secondary crt
		regs->vgaControl
			= crtid == 1 ? D2VGA_CONTROL : D1VGA_CONTROL;
		regs->grphEnable
			= crtid == 1 ? D2GRPH_ENABLE : D1GRPH_ENABLE;
		regs->grphControl
			= crtid == 1 ? D2GRPH_CONTROL : D1GRPH_CONTROL;
		regs->grphSwapControl
			= crtid == 1 ? D2GRPH_SWAP_CNTL : D1GRPH_SWAP_CNTL;
		regs->grphPrimarySurfaceAddr
			= crtid == 1 ? D2GRPH_PRIMARY_SURFACE_ADDRESS
				: D1GRPH_PRIMARY_SURFACE_ADDRESS;
		regs->grphSecondarySurfaceAddr
			= crtid == 1 ? D2GRPH_SECONDARY_SURFACE_ADDRESS
				: D1GRPH_SECONDARY_SURFACE_ADDRESS;

		// Surface Address high only used on r770+
		regs->grphPrimarySurfaceAddrHigh
			= crtid == 1 ? R700_D2GRPH_PRIMARY_SURFACE_ADDRESS_HIGH
				: R700_D1GRPH_PRIMARY_SURFACE_ADDRESS_HIGH;
		regs->grphSecondarySurfaceAddrHigh
			= crtid == 1 ? R700_D2GRPH_SECONDARY_SURFACE_ADDRESS_HIGH
				: R700_D1GRPH_SECONDARY_SURFACE_ADDRESS_HIGH;

		regs->grphPitch
			= crtid == 1 ? D2GRPH_PITCH : D1GRPH_PITCH;
		regs->grphSurfaceOffsetX
			= crtid == 1 ? D2GRPH_SURFACE_OFFSET_X : D1GRPH_SURFACE_OFFSET_X;
		regs->grphSurfaceOffsetY
			= crtid == 1 ? D2GRPH_SURFACE_OFFSET_Y : D1GRPH_SURFACE_OFFSET_Y;
		regs->grphXStart
			= crtid == 1 ? D2GRPH_X_START : D1GRPH_X_START;
		regs->grphYStart
			= crtid == 1 ? D2GRPH_Y_START : D1GRPH_Y_START;
		regs->grphXEnd
			= crtid == 1 ? D2GRPH_X_END : D1GRPH_X_END;
		regs->grphYEnd
			= crtid == 1 ? D2GRPH_Y_END : D1GRPH_Y_END;
		regs->crtControl
			= crtid == 1 ? D2CRTC_CONTROL : D1CRTC_CONTROL;
		regs->modeDesktopHeight
			= crtid == 1 ? D2MODE_DESKTOP_HEIGHT : D1MODE_DESKTOP_HEIGHT;
		regs->modeDataFormat
			= crtid == 1 ? D2MODE_DATA_FORMAT : D1MODE_DATA_FORMAT;
		regs->viewportStart
			= crtid == 1 ? D2MODE_VIEWPORT_START : D1MODE_VIEWPORT_START;
		regs->viewportSize
			= crtid == 1 ? D2MODE_VIEWPORT_SIZE : D1MODE_VIEWPORT_SIZE;
	} else {
		// this really shouldn't happen unless a driver PCIID chipset is wrong
		TRACE("%s, unknown Radeon chipset: r%X\n", __func__,
			info.device_chipset);
		return B_ERROR;
	}

	// Populate common registers
	// TODO : Wait.. this doesn't work with Eyefinity > crt 1.

	regs->modeCenter
		= crtid == 1 ? D2MODE_CENTER : D1MODE_CENTER;
	regs->grphUpdate
		= crtid == 1 ? D2GRPH_UPDATE : D1GRPH_UPDATE;
	regs->crtHPolarity
		= crtid == 1 ? D2CRTC_H_SYNC_A_CNTL : D1CRTC_H_SYNC_A_CNTL;
	regs->crtVPolarity
		= crtid == 1 ? D2CRTC_V_SYNC_A_CNTL : D1CRTC_V_SYNC_A_CNTL;
	regs->crtHTotal
		= crtid == 1 ? D2CRTC_H_TOTAL : D1CRTC_H_TOTAL;
	regs->crtVTotal
		= crtid == 1 ? D2CRTC_V_TOTAL : D1CRTC_V_TOTAL;
	regs->crtHSync
		= crtid == 1 ? D2CRTC_H_SYNC_A : D1CRTC_H_SYNC_A;
	regs->crtVSync
		= crtid == 1 ? D2CRTC_V_SYNC_A : D1CRTC_V_SYNC_A;
	regs->crtHBlank
		= crtid == 1 ? D2CRTC_H_BLANK_START_END : D1CRTC_H_BLANK_START_END;
	regs->crtVBlank
		= crtid == 1 ? D2CRTC_V_BLANK_START_END : D1CRTC_V_BLANK_START_END;
	regs->crtInterlace
		= crtid == 1 ? D2CRTC_INTERLACE_CONTROL : D1CRTC_INTERLACE_CONTROL;
	regs->crtCountControl
		= crtid == 1 ? D2CRTC_COUNT_CONTROL : D1CRTC_COUNT_CONTROL;
	regs->sclUpdate
		= crtid == 1 ? D2SCL_UPDATE : D1SCL_UPDATE;
	regs->sclEnable
		= crtid == 1 ? D2SCL_ENABLE : D1SCL_ENABLE;
	regs->sclTapControl
		= crtid == 1 ? D2SCL_TAP_CONTROL : D1SCL_TAP_CONTROL;

	TRACE("%s, registers for ATI chipset r%X crt #%d loaded\n", __func__,
		info.device_chipset, crtid);

	return B_OK;
}


status_t
detect_crt_ranges(uint32 crtid)
{
	edid1_info *edid = &gInfo->shared_info->edid_info;

	// TODO : VESA edid is just for primary monitor

	// EDID spec states 4 descriptor blocks
	for (uint32 index = 0; index < EDID1_NUM_DETAILED_MONITOR_DESC; index++) {

		edid1_detailed_monitor *monitor
			= &edid->detailed_monitor[index];

		if (monitor->monitor_desc_type
			== EDID1_MONITOR_RANGES) {
			edid1_monitor_range range = monitor->data.monitor_range;
			gDisplay[crtid]->vfreq_min = range.min_v;   /* in Hz */
			gDisplay[crtid]->vfreq_max = range.max_v;
			gDisplay[crtid]->hfreq_min = range.min_h;   /* in kHz */
			gDisplay[crtid]->hfreq_max = range.max_h;
			TRACE("CRT %d : v_min %d : v_max %d : h_min %d : h_max %d\n",
				crtid, gDisplay[crtid]->vfreq_min, gDisplay[crtid]->vfreq_max,
				gDisplay[crtid]->hfreq_min, gDisplay[crtid]->hfreq_max);
			return B_OK;
		}

	}

	return B_ERROR;
}


status_t
detect_displays()
{
	// reset known displays
	for (uint32 id = 0; id < MAX_DISPLAY; id++)
		gDisplay[id]->active = false;

	uint32 index = 0;

	// Probe for DAC monitors connected
	for (uint32 id = 0; id < 2; id++) {
		if (DACSense(id)) {
			gDisplay[index]->active = true;
			gDisplay[index]->connection_type = CONNECTION_DAC;
			gDisplay[index]->connection_id = id;
			init_registers(gDisplay[index]->regs, index);
			detect_crt_ranges(index);
			if (index < MAX_DISPLAY)
				index++;
			else
				return B_OK;
		}
	}

	// Probe for TMDS monitors connected
	for (uint32 id = 0; id < 1; id++) {
		if (TMDSSense(id)) {
			gDisplay[index]->active = true;
			gDisplay[index]->connection_type = CONNECTION_TMDS;
			gDisplay[index]->connection_id = id;
			init_registers(gDisplay[index]->regs, index);
			detect_crt_ranges(index);
			if (index < MAX_DISPLAY)
				index++;
			else
				return B_OK;
		}
	}

	return B_OK;
}


