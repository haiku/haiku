/*
 * Copyright 2008-2012 Haiku, Inc.  All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Gerald Zajac
 */

#include "accelerant.h"


extern "C" void*
get_accelerant_hook(uint32 feature, void* data)
{
	(void)data;		// avoid compiler warning for unused arg

	switch (feature) {
		// General
		case B_INIT_ACCELERANT:
			return (void*)InitAccelerant;
		case B_UNINIT_ACCELERANT:
			return (void*)UninitAccelerant;
		case B_CLONE_ACCELERANT:
			return (void*)CloneAccelerant;
		case B_ACCELERANT_CLONE_INFO_SIZE:
			return (void*)AccelerantCloneInfoSize;
		case B_GET_ACCELERANT_CLONE_INFO:
			return (void*)GetAccelerantCloneInfo;
		case B_GET_ACCELERANT_DEVICE_INFO:
			return (void*)GetAccelerantDeviceInfo;
		case B_ACCELERANT_RETRACE_SEMAPHORE:
			return NULL;

		// Mode Configuration
		case B_ACCELERANT_MODE_COUNT:
			return (void*)AccelerantModeCount;
		case B_GET_MODE_LIST:
			return (void*)GetModeList;
		case B_PROPOSE_DISPLAY_MODE:
			return (void*)ProposeDisplayMode;
		case B_SET_DISPLAY_MODE:
			return (void*)SetDisplayMode;
		case B_GET_DISPLAY_MODE:
			return (void*)GetDisplayMode;
#ifdef __HAIKU__
		case B_GET_EDID_INFO:
			return (void*)GetEdidInfo;
#endif
		case B_GET_FRAME_BUFFER_CONFIG:
			return (void*)GetFrameBufferConfig;
		case B_GET_PIXEL_CLOCK_LIMITS:
			return (void*)GetPixelClockLimits;
		case B_MOVE_DISPLAY:
			return (void*)MoveDisplay;
		case B_SET_INDEXED_COLORS:
			return (void*)(I810_SetIndexedColors);
		case B_GET_TIMING_CONSTRAINTS:
			return NULL;

		// DPMS
		case B_DPMS_CAPABILITIES:
			return (void*)(I810_DPMSCapabilities);
		case B_DPMS_MODE:
			return (void*)(I810_GetDPMSMode);
		case B_SET_DPMS_MODE:
			return (void*)(I810_SetDPMSMode);

		// Engine Management
		case B_ACCELERANT_ENGINE_COUNT:
			return (void*)AccelerantEngineCount;
		case B_ACQUIRE_ENGINE:
			return (void*)AcquireEngine;
		case B_RELEASE_ENGINE:
			return (void*)ReleaseEngine;
		case B_WAIT_ENGINE_IDLE:
			return (void*)WaitEngineIdle;
		case B_GET_SYNC_TOKEN:
			return (void*)GetSyncToken;
		case B_SYNC_TO_TOKEN:
			return (void*)SyncToToken;
	}

	return NULL;	// Return null pointer for any feature not handled above
}
