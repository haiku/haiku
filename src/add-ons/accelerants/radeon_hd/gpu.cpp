/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */


#include "accelerant_protos.h"
#include "accelerant.h"
#include "utility.h"
#include "gpu.h"

#include <Debug.h>

#undef TRACE

#define TRACE_GPU
#ifdef TRACE_GPU
#   define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#   define TRACE(x...) ;
#endif

#define ERROR(x...) _sPrintf("radeon_hd: " x)


status_t
radeon_gpu_reset()
{
	radeon_shared_info &info = *gInfo->shared_info;

	// Read GRBM Command Processor status
	if (!(Read32(OUT, GRBM_STATUS) & GUI_ACTIVE))
		return B_ERROR;

	TRACE("%s: GPU software reset in progress...\n", __func__);

	// TODO : mc stop

	if (radeon_gpu_mc_idle() > 0) {
		ERROR("%s: Timeout waiting for MC to idle!\n", __func__);
	}

	if (info.device_chipset < RADEON_R1000) {
		Write32(OUT, CP_ME_CNTL, CP_ME_HALT);
			// Disable Command Processor parsing / prefetching

		// Register busy masks for early Radeon HD cards

		// GRBM Command Processor Status
		uint32 grbm_busy_mask = VC_BUSY;
			// Vertex Cache Busy
		grbm_busy_mask |= VGT_BUSY_NO_DMA | VGT_BUSY;
			// Vertex Grouper Tessellator Busy
		grbm_busy_mask |= TA03_BUSY;
			// unknown
		grbm_busy_mask |= TC_BUSY;
			// Texture Cache Busy
		grbm_busy_mask |= SX_BUSY;
			// Shader Export Busy
		grbm_busy_mask |= SH_BUSY;
			// Sequencer Instruction Cache Busy
		grbm_busy_mask |= SPI_BUSY;
			// Shader Processor Interpolator Busy
		grbm_busy_mask |= SMX_BUSY;
			// Shader Memory Exchange
		grbm_busy_mask |= SC_BUSY;
			// Scan Converter Busy
		grbm_busy_mask |= PA_BUSY;
			// Primitive Assembler Busy
		grbm_busy_mask |= DB_BUSY;
			// Depth Block Busy
		grbm_busy_mask |= CR_BUSY;
			// unknown
		grbm_busy_mask |= CB_BUSY;
			// Color Block Busy
		grbm_busy_mask |= GUI_ACTIVE;
			// unknown (graphics pipeline active?)

		// GRBM Command Processor Detailed Status
		uint32 grbm2_busy_mask = SPI0_BUSY | SPI1_BUSY | SPI2_BUSY | SPI3_BUSY;
			// Shader Processor Interpolator 0 - 3 Busy
		grbm2_busy_mask |= TA0_BUSY | TA1_BUSY | TA2_BUSY | TA3_BUSY;
			// unknown 0 - 3 Busy
		grbm2_busy_mask |= DB0_BUSY | DB1_BUSY | DB2_BUSY | DB3_BUSY;
			// Depth Block 0 - 3 Busy
		grbm2_busy_mask |= CB0_BUSY | CB1_BUSY | CB2_BUSY | CB3_BUSY;
			// Color Block 0 - 3 Busy

		uint32 tmp;
		/* Check if any of the rendering block is busy and reset it */
		if ((Read32(OUT, GRBM_STATUS) & grbm_busy_mask)
			|| (Read32(OUT, GRBM_STATUS2) & grbm2_busy_mask)) {
			tmp = SOFT_RESET_CR
				| SOFT_RESET_DB
				| SOFT_RESET_CB
				| SOFT_RESET_PA
				| SOFT_RESET_SC
				| SOFT_RESET_SMX
				| SOFT_RESET_SPI
				| SOFT_RESET_SX
				| SOFT_RESET_SH
				| SOFT_RESET_TC
				| SOFT_RESET_TA
				| SOFT_RESET_VC
				| SOFT_RESET_VGT;
			Write32(OUT, GRBM_SOFT_RESET, tmp);
			Read32(OUT, GRBM_SOFT_RESET);
			snooze(15000);
			Write32(OUT, GRBM_SOFT_RESET, 0);
		}

		// Reset CP
		tmp = SOFT_RESET_CP;
		Write32(OUT, GRBM_SOFT_RESET, tmp);
		Read32(OUT, GRBM_SOFT_RESET);
		snooze(15000);
		Write32(OUT, GRBM_SOFT_RESET, 0);

		// Let things settle
		snooze(1000);
	} else {
		// Northern Islands and higher

		Write32(OUT, CP_ME_CNTL, CP_ME_HALT | CP_PFP_HALT);
			// Disable Command Processor parsing / prefetching

		// reset the graphics pipeline components
		uint32 grbm_reset = (SOFT_RESET_CP
			| SOFT_RESET_CB
			| SOFT_RESET_DB
			| SOFT_RESET_GDS
			| SOFT_RESET_PA
			| SOFT_RESET_SC
			| SOFT_RESET_SPI
			| SOFT_RESET_SH
			| SOFT_RESET_SX
			| SOFT_RESET_TC
			| SOFT_RESET_TA
			| SOFT_RESET_VGT
			| SOFT_RESET_IA);

		Write32(OUT, GRBM_SOFT_RESET, grbm_reset);
		Read32(OUT, GRBM_SOFT_RESET);

		snooze(50);
		Write32(OUT, GRBM_SOFT_RESET, 0);
		Read32(OUT, GRBM_SOFT_RESET);
		snooze(50);
	}


	// TODO : mc resume
	return B_OK;
}


uint32
radeon_gpu_mc_idle()
{
	uint32 idleStatus;
	if (!((idleStatus = Read32(MC, SRBM_STATUS)) &
		(VMC_BUSY | MCB_BUSY |
			MCDZ_BUSY | MCDY_BUSY | MCDX_BUSY | MCDW_BUSY)))
		return 0;

	return idleStatus;
}


status_t
radeon_gpu_mc_setup()
{
	uint32 fb_location_int = gInfo->shared_info->frame_buffer_int;

	uint32 fb_location = Read32(OUT, R6XX_MC_VM_FB_LOCATION);
	uint16 fb_size = (fb_location >> 16) - (fb_location & 0xFFFF);
	uint32 fb_location_tmp = fb_location_int >> 24;
	fb_location_tmp |= (fb_location_tmp + fb_size) << 16;
	uint32 fb_offset_tmp = (fb_location_int >> 8) & 0xff0000;

	uint32 idleState = radeon_gpu_mc_idle();
	if (idleState > 0) {
		TRACE("%s: Cannot modify non-idle MC! idleState: 0x%" B_PRIX32 "\n",
			__func__, idleState);
		return B_ERROR;
	}

	TRACE("%s: Setting frame buffer from 0x%" B_PRIX32
		" to 0x%" B_PRIX32 " [size 0x%" B_PRIX16 "]\n",
		__func__, fb_location, fb_location_tmp, fb_size);

	// The MC Write32 will handle cards needing a special MC read/write register
	Write32(MC, R6XX_MC_VM_FB_LOCATION, fb_location_tmp);
	Write32(MC, R6XX_HDP_NONSURFACE_BASE, fb_offset_tmp);

	return B_OK;
}


status_t
radeon_gpu_irq_setup()
{
	// TODO : Stub for IRQ setup

	// allocate rings via r600_ih_ring_alloc

	// disable irq's via r600_disable_interrupts

	// r600_rlc_init

	// setup interrupt control

	return B_ERROR;
}
