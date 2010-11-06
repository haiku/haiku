/*
 * Copyright 2007-2010 Haiku, Inc.  All rights reserved.
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
#	define TRACE(x...) _sPrintf("3dfx: " x)
#else
#	define TRACE(x...) ;
#endif


#define CURSOR_BYTES	1024		// bytes used for cursor image in video memory


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
void	 TDFX_SetIndexedColors(uint count, uint8 first, uint8* color_data,
			uint32 flags);
status_t GetEdidInfo(void* info, size_t size, uint32* _version);

// DPMS
uint32	 TDFX_DPMSCapabilities(void);
uint32	 TDFX_GetDPMSMode(void);
status_t TDFX_SetDPMSMode(uint32 dpms_flags);

// Cursor
status_t SetCursorShape(uint16 width, uint16 height, uint16 hot_x, uint16 hot_y,
			uint8* andMask, uint8* xorMask);
void	 MoveCursor(uint16 x, uint16 y);
void	 TDFX_ShowCursor(bool bShow);

// Engine Management
uint32	 AccelerantEngineCount(void);
status_t AcquireEngine(uint32 capabilities, uint32 max_wait, sync_token* st,
			engine_token** et);
status_t ReleaseEngine(engine_token* et, sync_token* st);
void	 WaitEngineIdle(void);
status_t GetSyncToken(engine_token* et, sync_token* st);
status_t SyncToToken(sync_token* st);

// 2D acceleration
void	 TDFX_FillRectangle(engine_token* et, uint32 color,
			fill_rect_params* list, uint32 count);
void	 TDFX_FillSpan(engine_token* et, uint32 color, uint16* list, 
			uint32 count);
void	 TDFX_InvertRectangle(engine_token* et, fill_rect_params* list, 
			uint32 count);
void	 TDFX_ScreenToScreenBlit(engine_token* et, blit_params* list, 
			uint32 count);

// Video_overlay
uint32		OverlayCount(const display_mode* dm);
const uint32* OverlaySupportedSpaces(const display_mode *dm);
uint32		OverlaySupportedFeatures(uint32 a_color_space);
const overlay_buffer* AllocateOverlayBuffer(color_space cs, uint16 width,
				uint16 height);
status_t	ReleaseOverlayBuffer(const overlay_buffer* ob);
status_t	GetOverlayConstraints(const display_mode* dm,
				const overlay_buffer* ob, overlay_constraints* oc);
overlay_token AllocateOverlay(void);
status_t	ReleaseOverlay(overlay_token ot);
status_t	ConfigureOverlay(overlay_token ot, const overlay_buffer* ob,
				const overlay_window* ow, const overlay_view* ov);

#if defined(__cplusplus)
}
#endif



// Prototypes for other functions that are called from source files other than
// where they are defined.
//============================================================================

status_t CreateModeList(bool (*checkMode)(const display_mode* mode));
uint16	 GetVesaModeNumber(const display_mode& mode, uint8 bitsPerPixel);
bool	 IsModeUsable(const display_mode* mode);

// 3dfx functions.

bool	 TDFX_DisplayOverlay(const overlay_window* window,
			const overlay_buffer* buffer, const overlay_view* view);
void	 TDFX_StopOverlay(void);

status_t TDFX_Init(void);
bool	 TDFX_GetColorSpaceParams(int colorSpace, uint8& bpp);
bool	 TDFX_GetEdidInfo(edid1_info& edidInfo);

void	 TDFX_EngineReset(void);
void	 TDFX_EngineInit(const DisplayModeEx& mode);

bool	 TDFX_LoadCursorImage(int width, int height, uint8* and_mask, 
			uint8* xor_mask);
void	 TDFX_SetCursorPosition(int x, int y);

void	 TDFX_AdjustFrame(const DisplayModeEx& mode);
status_t TDFX_SetDisplayMode(const DisplayModeEx& mode);

void	 TDFX_WaitForFifo(uint32);
void	 TDFX_WaitForIdle();


// Address of various VGA registers.

#define MISC_OUT_R		0x3cc		// read
#define MISC_OUT_W		0x3c2		// write
#define CRTC_INDEX		0x3d4
#define CRTC_DATA		0x3d5
#define SEQ_INDEX		0x3c4
#define SEQ_DATA		0x3c5


// Macros for memory mapped I/O.
//===============================

#define INREG8(addr)		*((vuint8*)(gInfo.regs + addr))
#define INREG16(addr)		*((vuint16*)(gInfo.regs + addr))
#define INREG32(addr)		*((vuint32*)(gInfo.regs + addr))

#define OUTREG8(addr, val)	*((vuint8*)(gInfo.regs + addr)) = val
#define OUTREG16(addr, val)	*((vuint16*)(gInfo.regs + addr)) = val
#define OUTREG32(addr, val)	*((vuint32*)(gInfo.regs + addr)) = val

// Write a value to an 32-bit reg using a mask.  The mask selects the
// bits to be modified.
#define OUTREGM(addr, value, mask)	\
	(OUTREG(addr, (INREG(addr) & ~(mask)) | ((value) & (mask))))


#endif	// _ACCELERANT_H
