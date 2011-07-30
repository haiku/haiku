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
#include "mc.h"


/*		Read32/Write32 for MC can hande the fact that some cards need to write
		to a special MC interface, while others just write to the card directly.
		As of R600 - R800 though the special MC interface doesn't seem to exist
*/


#define TRACE_MC
#ifdef TRACE_MC
extern "C" void _sPrintf(const char *format, ...);
#   define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#   define TRACE(x...) ;
#endif


uint32
MCIdle()
{
	uint32 turboencabulator;
	if (!((turboencabulator = Read32(MC, SRBM_STATUS)) &
		(VMC_BUSY_bit | MCB_BUSY_bit |
			MCDZ_BUSY_bit | MCDY_BUSY_bit | MCDX_BUSY_bit | MCDW_BUSY_bit)))
		return 0;

	return turboencabulator;
}


status_t
MCFBSetup()
{
	uint32 fb_location_int = gInfo->shared_info->frame_buffer_int;

	uint32 fb_location = Read32(OUT, R6XX_MC_VM_FB_LOCATION);
	uint16 fb_size = (fb_location >> 16) - (fb_location & 0xFFFF);
	uint32 fb_location_tmp = fb_location_int >> 24;
	fb_location_tmp |= (fb_location_tmp + fb_size) << 16;
	uint32 fb_offset_tmp = (fb_location_int >> 8) & 0xff0000;

	uint32 idleState = MCIdle();
	if (idleState > 0) {
		TRACE("%s: Cannot modify non-idle MC! idleState: %X\n",
			__func__, idleState);
		return B_ERROR;
	}

	TRACE("%s: Setting frame buffer from 0x%08X to 0x%08X [size 0x%08X]\n",
		__func__, fb_location, fb_location_tmp, fb_size);

	// The MC Write32 will handle cards needing a special MC read/write register
	Write32(MC, R6XX_MC_VM_FB_LOCATION, fb_location_tmp);
	Write32(MC, R6XX_HDP_NONSURFACE_BASE, fb_offset_tmp);

	return B_OK;
}
