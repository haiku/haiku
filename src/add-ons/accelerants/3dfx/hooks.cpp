/*
 * Copyright 2008-2010 Haiku, Inc.  All rights reserved.
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
		case B_GET_PREFERRED_DISPLAY_MODE:
			return NULL;
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
			return (void*)(TDFX_SetIndexedColors);
		case B_GET_TIMING_CONSTRAINTS:
			return NULL;

		// DPMS
		case B_DPMS_CAPABILITIES:
			return (void*)(TDFX_DPMSCapabilities);
		case B_DPMS_MODE:
			return (void*)(TDFX_GetDPMSMode);
		case B_SET_DPMS_MODE:
			return (void*)(TDFX_SetDPMSMode);

		// Cursor
		case B_SET_CURSOR_SHAPE:
			return (void*)SetCursorShape;
		case B_MOVE_CURSOR:
			return (void*)MoveCursor;
		case B_SHOW_CURSOR:
			return (void*)(TDFX_ShowCursor);

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

		// 2D acceleration
		case B_SCREEN_TO_SCREEN_BLIT:
			return (void*)(TDFX_ScreenToScreenBlit);
		case B_FILL_RECTANGLE:
			return (void*)(TDFX_FillRectangle);
		case B_INVERT_RECTANGLE:
			return (void*)(TDFX_InvertRectangle);
		case B_FILL_SPAN:
			return (void*)(TDFX_FillSpan);

		// Overlay
		case B_OVERLAY_COUNT:
			return (void*)OverlayCount;
		case B_OVERLAY_SUPPORTED_SPACES:
			return (void*)OverlaySupportedSpaces;
		case B_OVERLAY_SUPPORTED_FEATURES:
			return (void*)OverlaySupportedFeatures;
		case B_ALLOCATE_OVERLAY_BUFFER:
			return (void*)AllocateOverlayBuffer;
		case B_RELEASE_OVERLAY_BUFFER:
			return (void*)ReleaseOverlayBuffer;
		case B_GET_OVERLAY_CONSTRAINTS:
			return (void*)GetOverlayConstraints;
		case B_ALLOCATE_OVERLAY:
			return (void*)AllocateOverlay;
		case B_RELEASE_OVERLAY:
			return (void*)ReleaseOverlay;
		case B_CONFIGURE_OVERLAY:
			return (void*)ConfigureOverlay;
	}

	return NULL;	// Return null pointer for any feature not handled above
}
