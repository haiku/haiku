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


uint64
MCFBLocation(uint16 chipset, uint32* size)
{
	// TODO : R800 : This is only valid for all R6xx and R7xx?
	uint32 fbLocationReg = Read32(MC, R7XX_MC_VM_FB_LOCATION);
	*size = (((fbLocationReg & 0xFFFF0000)
		- ((fbLocationReg & 0xFFFF) << 16))) << 8;
	return (fbLocationReg & 0xFFFF) << 24;
}


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
MCFBSetup(uint32 newFbLocation, uint32 newFbSize)
{
	uint32 oldFbSize;
	uint64 oldFbLocation = MCFBLocation(0, &oldFbSize);

	if (oldFbLocation == newFbLocation
		&& oldFbSize == newFbSize) {
		TRACE("%s: not adjusting frame buffer as it is already correct\n",
			__func__);
		return B_OK;
	}

	if (oldFbLocation >> 32) {
		TRACE("%s: board claims to use a frame buffer address > 32-bits\n",
			__func__);
	}

	uint32 idleState = MCIdle();
	if (idleState > 0) {
		TRACE("%s: Cannot modify non-idle MC! idleState: %X\n",
			__func__, idleState);
		return B_ERROR;
	}

	TRACE("%s: Setting MC/FB from 0x%08X to 0x%08X [size 0x%08X]\n",
		__func__, oldFbLocation, newFbLocation, newFbSize);

	// The MC Write32 will handle cards needing a special MC read/write register
	Write32(MC, R6XX_MC_VM_FB_LOCATION,
		R6XX_FB_LOCATION(newFbLocation, newFbSize));
	Write32(MC, R6XX_HDP_NONSURFACE_BASE, R6XX_HDP_LOCATION(newFbLocation));

	return B_OK;
}
