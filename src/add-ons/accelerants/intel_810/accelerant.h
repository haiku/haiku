/*
 * Copyright 2007-2012 Haiku, Inc.  All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Gerald Zajac
 */

#ifndef _ACCELERANT_H
#define _ACCELERANT_H

#include "DriverInterface.h"



#undef TRACE

#ifdef ENABLE_DEBUG_TRACE
extern "C" void  _sPrintf(const char* format, ...);
#	define TRACE(x...) _sPrintf("i810: " x)
#else
#	define TRACE(x...) ;
#endif


// Global data used by various source files of the accelerant.

struct AccelerantInfo {
	int			deviceFileDesc;		// file descriptor of kernel driver

	SharedInfo*	sharedInfo;			// address of info shared between
									// accelerants & driver
	area_id		sharedInfoArea;		// shared info area ID

	uint8*		regs;				// base address of MMIO register area
	area_id		regsArea;			// MMIO register area ID

	display_mode* modeList;			// list of standard display modes
	area_id		modeListArea;		// mode list area ID

	bool		bAccelerantIsClone;	// true if this is a cloned accelerant
};

extern AccelerantInfo gInfo;


// Prototypes of the interface functions called by the app_server.  Note that
// the functions that are unique to a particular chip family, will be prefixed
// with the name of the family, and the functions that are applicable to all
// chips will have no prefix.
//================================================================

#if defined(__cplusplus)
extern "C" {
#endif

// General
status_t InitAccelerant(int fd);
ssize_t  AccelerantCloneInfoSize(void);
void	 GetAccelerantCloneInfo(void* data);
status_t CloneAccelerant(void* data);
void	 UninitAccelerant(void);
status_t GetAccelerantDeviceInfo(accelerant_device_info* adi);

// Mode Configuration
uint32	 AccelerantModeCount(void);
status_t GetModeList(display_mode* dm);
status_t ProposeDisplayMode(display_mode* target, const display_mode* low,
			const display_mode* high);
status_t SetDisplayMode(display_mode* mode_to_set);
status_t GetDisplayMode(display_mode* current_mode);
status_t GetFrameBufferConfig(frame_buffer_config* a_frame_buffer);
status_t GetPixelClockLimits(display_mode* dm, uint32* low, uint32* high);
status_t MoveDisplay(uint16 h_display_start, uint16 v_display_start);
void	 I810_SetIndexedColors(uint count, uint8 first, uint8* color_data,
			uint32 flags);
status_t GetEdidInfo(void* info, size_t size, uint32* _version);

// DPMS
uint32   I810_DPMSCapabilities(void);
uint32   I810_GetDPMSMode(void);
status_t I810_SetDPMSMode(uint32 dpms_flags);

// Engine Management
uint32   AccelerantEngineCount(void);
status_t AcquireEngine(uint32 capabilities, uint32 max_wait, sync_token* st,
			engine_token** et);
status_t ReleaseEngine(engine_token* et, sync_token* st);
void	 WaitEngineIdle(void);
status_t GetSyncToken(engine_token* et, sync_token* st);
status_t SyncToToken(sync_token* st);

#if defined(__cplusplus)
}
#endif



// Prototypes for other functions that are called from source files other than
// where they are defined.
//============================================================================

status_t CreateModeList(bool (*checkMode)(const display_mode* mode));
bool	 IsModeUsable(const display_mode* mode);

// Intel 810 functions.

status_t I810_Init(void);
bool	 I810_GetColorSpaceParams(int colorSpace, uint8& bpp,
			uint32& maxPixelClk);
uint32	 I810_GetWatermark(const DisplayModeEx& mode);

void	 I810_AdjustFrame(const DisplayModeEx& mode);
status_t I810_SetDisplayMode(const DisplayModeEx& mode);


#endif	// _ACCELERANT_H
