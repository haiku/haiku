/*
	Copyright 2007-2008 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2007-2008
*/

#ifndef _ACCEL_H
#define _ACCEL_H

#include "DriverInterface.h"
#include "register_io.h"



#undef TRACE

#ifdef ENABLE_DEBUG_TRACE
extern "C" void  _sPrintf(const char* format, ...);
#	define TRACE(x...) _sPrintf("S3: " x)
#else
#	define TRACE(x...) ;
#endif


// Global data used by various source files of the accelerant.

struct AccelerantInfo {
	int			 deviceFileDesc;	// file descriptor of kernel driver

	SharedInfo*	 sharedInfo;		// address of info shared between accelerants & driver
	area_id		 sharedInfoArea;	// shared info area ID

	uint8*		 regs;				// base address of MMIO register area
	area_id		 regsArea;			// MMIO register area ID

	display_mode* modeList;			// list of standard display modes
	area_id		 modeListArea;		// mode list area ID

	bool	bAccelerantIsClone;		// true if this is a cloned accelerant

	// Pointers to wait handlers.
	void	(*WaitQueue)(uint32);
	void	(*WaitIdleEmpty)();

	// Pointers to DPMS functions.
	uint32	(*DPMSCapabilities)(void);
	uint32	(*GetDPMSMode)(void);
	status_t (*SetDPMSMode)(uint32 dpms_flags);

	// Pointers to cursor functions.
	bool	(*LoadCursorImage)(int width, int height, uint8* and_mask, uint8* xor_mask);
	void	(*SetCursorPosition)(int x, int y);
	void	(*ShowCursor)(bool bShow);

	// Pointers to 2D acceleration functions.
	void	(*FillRectangle)(engine_token*, uint32 color, fill_rect_params*, uint32 count);
	void	(*FillSpan)(engine_token*, uint32 color, uint16* list, uint32 count);
	void	(*InvertRectangle)(engine_token*, fill_rect_params*, uint32 count);
	void	(*ScreenToScreenBlit)(engine_token*, blit_params*, uint32 count);

	// Pointers to other functions.
	void	(*AdjustFrame)(const DisplayModeEx& mode);
	status_t (*ChipInit)(void);
	bool	(*GetColorSpaceParams)(int colorSpace, uint32& bpp, uint32& maxPixelClk);
	bool	(*SetDisplayMode)(const DisplayModeEx& mode);
	void	(*SetIndexedColors)(uint count, uint8 first, uint8* color_data, uint32 flags);
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
sem_id	 AccelerantRetraceSemaphore(void);

// Mode Configuration
uint32	 AccelerantModeCount(void);
status_t GetModeList(display_mode* dm);
status_t ProposeDisplayMode(display_mode* target, const display_mode* low, const display_mode* high);
status_t SetDisplayMode(display_mode* mode_to_set);
status_t GetDisplayMode(display_mode* current_mode);
status_t GetFrameBufferConfig(frame_buffer_config* a_frame_buffer);
status_t GetPixelClockLimits(display_mode* dm, uint32* low, uint32* high);
status_t MoveDisplay(uint16 h_display_start, uint16 v_display_start);
status_t GetTimingConstraints(display_timing_constraints* dtc);
void	 Savage_SetIndexedColors(uint count, uint8 first, uint8* color_data, uint32 flags);
void	 Trio64_SetIndexedColors(uint count, uint8 first, uint8* color_data, uint32 flags);
void	 Virge_SetIndexedColors(uint count, uint8 first, uint8* color_data, uint32 flags);
status_t GetPreferredDisplayMode(display_mode* preferredMode);
status_t GetEdidInfo(void* info, size_t size, uint32* _version);

// DPMS
uint32   Savage_DPMSCapabilities(void);
uint32   Savage_GetDPMSMode(void);
status_t Savage_SetDPMSMode(uint32 dpms_flags);

uint32   Trio64_DPMSCapabilities(void);
uint32   Trio64_GetDPMSMode(void);
status_t Trio64_SetDPMSMode(uint32 dpms_flags);

uint32   Virge_DPMSCapabilities(void);
uint32   Virge_GetDPMSMode(void);
status_t Virge_SetDPMSMode(uint32 dpms_flags);

// Cursor
status_t SetCursorShape(uint16 width, uint16 height, uint16 hot_x, uint16 hot_y,
						uint8* andMask, uint8* xorMask);
void	 MoveCursor(uint16 x, uint16 y);
void	 Savage_ShowCursor(bool bShow);
void	 Trio64_ShowCursor(bool bShow);
void	 Virge_ShowCursor(bool bShow);

// Engine Management
uint32   AccelerantEngineCount(void);
status_t AcquireEngine(uint32 capabilities, uint32 max_wait, sync_token* st, engine_token** et);
status_t ReleaseEngine(engine_token* et, sync_token* st);
void	 WaitEngineIdle(void);
status_t GetSyncToken(engine_token* et, sync_token* st);
status_t SyncToToken(sync_token* st);

// 2D acceleration
void	 Savage_FillRectangle(engine_token* et, uint32 color, fill_rect_params* list, uint32 count);
void	 Savage_FillSpan(engine_token* et, uint32 color, uint16* list, uint32 count);
void	 Savage_InvertRectangle(engine_token* et, fill_rect_params* list, uint32 count);
void	 Savage_ScreenToScreenBlit(engine_token* et, blit_params* list, uint32 count);

void	 Trio64_FillRectangle(engine_token* et, uint32 color, fill_rect_params* list, uint32 count);
void	 Trio64_FillSpan(engine_token* et, uint32 color, uint16* list, uint32 count);
void	 Trio64_InvertRectangle(engine_token* et, fill_rect_params* list, uint32 count);
void	 Trio64_ScreenToScreenBlit(engine_token* et, blit_params* list, uint32 count);

void	 Virge_FillRectangle(engine_token* et, uint32 color, fill_rect_params* list, uint32 count);
void	 Virge_FillSpan(engine_token* et, uint32 color, uint16* list, uint32 count);
void	 Virge_InvertRectangle(engine_token* et, fill_rect_params* list, uint32 count);
void	 Virge_ScreenToScreenBlit(engine_token* et, blit_params* list, uint32 count);

#if defined(__cplusplus)
}
#endif



// Prototypes for other functions that are called from source files other than
// where they are defined.
//============================================================================

status_t CreateModeList(bool (*checkMode)(const display_mode* mode),
						bool (*getEdid)(edid1_info& edidInfo));
void	 InitCrtcTimingValues(const DisplayModeEx& mode, int horzScaleFactor, uint8 crtc[],
							  uint8& cr3b, uint8& cr3c, uint8& cr5d, uint8& cr5e);
bool	 IsModeUsable(const display_mode* mode);

// Savage functions.

bool	 Savage_GetEdidInfo(edid1_info& edidInfo);

bool	 Savage_LoadCursorImage(int width, int height, uint8* and_mask, uint8* xor_mask);
void	 Savage_SetCursorPosition(int x, int y);

void	 Savage_AdjustFrame(const DisplayModeEx& mode);
bool	 Savage_SetDisplayMode(const DisplayModeEx& mode);
void	 Savage_SetFunctionPointers(void);

// Trio64 functions.

bool	 Trio64_LoadCursorImage(int width, int height, uint8* and_mask, uint8* xor_mask);
void	 Trio64_SetCursorPosition(int x, int y);

void	 Trio64_AdjustFrame(const DisplayModeEx& mode);
bool	 Trio64_SetDisplayMode(const DisplayModeEx& mode);
void	 Trio64_SetFunctionPointers(void);

// Virge functions.

bool	 Virge_GetEdidInfo(edid1_info& edidInfo);

bool	 Virge_LoadCursorImage(int width, int height, uint8* and_mask, uint8* xor_mask);
void	 Virge_SetCursorPosition(int x, int y);

void	 Virge_AdjustFrame(const DisplayModeEx& mode);
bool	 Virge_SetDisplayMode(const DisplayModeEx& mode);
void	 Virge_SetFunctionPointers(void);


#endif	// _ACCEL_H
